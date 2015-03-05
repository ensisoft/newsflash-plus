// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
// 
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define LOGTAG "rss"

#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtGui/QIcon>
#  include <QtXml/QDomDocument>
#  include <QIODevice>
#  include <QDateTime>
#  include <QUrl>
#  include <QByteArray>
#  include <QBuffer>
#  include <QDir>
#include <newsflash/warnpop.h>
#include "rssreader.h"
#include "eventlog.h"
#include "debug.h"
#include "format.h"
#include "nzbparse.h"
#include "womble.h"
#include "nzbs.h"
#include "nzbparse.h"
#include "engine.h"
#include "platform.h"
#include "types.h"

namespace app
{

RSSReader::RSSReader()
{
    net_ = g_net->getSubmissionContext();

    feeds_.emplace_back(new Womble);
    feeds_.emplace_back(new Nzbs);
    INFO("RSS http://nzbs.org");
    INFO("RSS http://newshost.co.za");

    net_.callback = [=] {
        on_ready();
    };

    DEBUG("RSSReader created");
}

RSSReader::~RSSReader()
{
    DEBUG("RSSReader destroyed");
}

QVariant RSSReader::data(const QModelIndex& index, int role) const
{
    const auto row = index.row();
    const auto col = columns(index.column());        
    const auto& item = items_[row];

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
            case columns::date:     return toString(app::event {item.pubdate}); 
            case columns::category: return toString(item.type);            
            case columns::locked:   return "";
            case columns::title:    return item.title;
            case columns::size:
            if (item.size == 0)
                    return "n/a";
                return toString(size { item.size });

            case columns::sentinel: Q_ASSERT(0);
        }
    }
    if (role == Qt::DecorationRole)
    {
        if (col  == columns::date)
            return QIcon("icons:ico_rss.png");
        else if (col == columns::locked && item.password)
            return QIcon("icons:ico_password.png");
    }
    return {};
}

QVariant RSSReader::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    if (role == Qt::DisplayRole)
    {
        switch (columns(section))
        {
            case columns::date:     return "Date";
            case columns::locked:   return "";
            case columns::category: return "Category";
            case columns::size:     return "Size";
            case columns::title:    return "Title";
            case columns::sentinel: Q_ASSERT(false); break;
        }
    }
    if (role == Qt::DecorationRole)
    {
        static const auto icoLock(toGrayScale("icons:ico_password.png"));
        if (columns(section) == columns::locked)
            return icoLock;
    }
    return {};
}

void RSSReader::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();

    #define SORT(x) \
        std::sort(std::begin(items_), std::end(items_), \
            [&](const MediaItem& lhs, const MediaItem& rhs) { \
                if (order == Qt::AscendingOrder) \
                    return lhs.x < rhs.x; \
                return lhs.x > rhs.x; \
            });
        switch (columns(column))
        {
            case columns::date:     SORT(pubdate); break;
            case columns::category: SORT(type);    break;
            case columns::size:     SORT(size);    break;
            case columns::locked:   SORT(password); break;
            case columns::title:    SORT(title);   break;
            case columns::sentinel: Q_ASSERT(false); break;
        }
    #undef SORT

    emit layoutChanged();
}

int RSSReader::rowCount(const QModelIndex&) const
{
    return (int)items_.size();
}

int RSSReader::columnCount(const QModelIndex&) const
{
    return (int)columns::sentinel;
}

void RSSReader::clear()
{
    items_.clear();
    QAbstractTableModel::reset();                
}

bool RSSReader::refresh(MediaType type)
{
    bool ret = false;

    for (auto& feed : feeds_)
    {
        if (!feed->isEnabled())
            continue;

        std::vector<QUrl> urls;
        feed->prepare(type, urls);
        for (const auto& url : urls)
        {
            g_net->submit(std::bind(&RSSReader::onRefreshComplete, this, feed.get(), type,
                std::placeholders::_1), net_, url);
            ret = true;
        }
    }

    return ret;
}

void RSSReader::enableFeed(const QString& feed, bool on_off)
{
    for (auto& f :feeds_)
    {
        if (f->name() == feed)
        {
            f->enable(on_off);
            return;
        }
    }
}

void RSSReader::setCredentials(const QString& feed, const QString& user, const QString& apikey)
{
    RSSFeed::params p;
    p.user = user;
    p.key  = apikey;
    for (auto& f : feeds_)
    {
        if (f->name() == feed)
        {
            f->setParams(p);
            return;
        }
    }
}


void RSSReader::downloadNzbFile(std::size_t index, const QString& file)
{
    BOUNDSCHECK(items_, index);

    const auto& item = items_[index];
    const auto& link = item.nzblink;

    g_net->submit(std::bind(&RSSReader::onNzbFileComplete, this, file,
        std::placeholders::_1), net_, link);
}

void RSSReader::downloadNzbFile(std::size_t index, data_callback cb)
{
    BOUNDSCHECK(items_, index);

    const auto& item = items_[index];
    const auto& link = item.nzblink;

    g_net->submit(std::bind(&RSSReader::onNzbDataCompleteCallback, this, std::move(cb), 
        std::placeholders::_1), net_, link);
}

void RSSReader::downloadNzbContent(std::size_t index, quint32 account, const QString& folder)
{
    BOUNDSCHECK(items_, index);

    const auto& item = items_[index];
    const auto& link = item.nzblink;
    const auto& desc = item.title;
    g_net->submit(std::bind(&RSSReader::onNzbDataComplete, this, folder, desc, account,
        std::placeholders::_1), net_, link);
}

void RSSReader::stop()
{
    g_net->cancel(net_);
}

void RSSReader::onRefreshComplete(RSSFeed* feed, MediaType type, QNetworkReply& reply)
{
    DEBUG("RSS stream reply %1", reply);

    const auto err = reply.error();
    const auto url = reply.url();
    if (err != QNetworkReply::NoError)
    {
        ERROR("RSS refresh failed (%1), %2", url, err);
        return;
    }
    QByteArray bytes = reply.readAll();
    QBuffer io(&bytes);

    //qDebug() << bytes.size();
    //qDebug() << bytes;

    std::vector<MediaItem> items;

    if (!feed->parse(io, items))
    {
        ERROR("RSS XML parse failed (%1)", url);
        return;
    }

    for (auto& i : items)
    {
        i.type = type;
        i.pubdate = i.pubdate.toLocalTime();
    }

    const auto count = items.size();

    beginInsertRows(QModelIndex(), items_.size(), items_.size() + count);
    std::copy(std::begin(items), std::end(items),
        std::back_inserter(items_));
    endInsertRows();
}

void RSSReader::onNzbFileComplete(const QString& file, QNetworkReply& reply)
{
    DEBUG("Get nzb data reply %1", reply);

    const auto err = reply.error();
    const auto url = reply.url();
    if (err != QNetworkReply::NoError)
    {
        ERROR("Failed to retrieve NZB file (%1), %2", url, err);
        return;
    }
    QFile io(file);
    if (!io.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ERROR("Failed to open file %1 for writing, %2", file, io.error());
        return;
    }

    QByteArray bytes = reply.readAll();
    io.write(bytes);
    io.close();
    INFO("Saved NZB file %1 %2", file, size {(unsigned)bytes.size()});
}

void RSSReader::onNzbDataComplete(const QString& folder, const QString& desc, quint32 acc, QNetworkReply& reply)
{
    DEBUG("Get nzb data reply %1", reply);

    const auto err = reply.error();
    const auto url = reply.url();
    if (err != QNetworkReply::NoError)
    {
        ERROR("Failed to retrieve NZB content (%1), %2", url, err);
        return;
    }

    QByteArray bytes = reply.readAll();

    g_engine->downloadNzbContents(acc, folder, desc, bytes);
}

void RSSReader::onNzbDataCompleteCallback(const data_callback& cb, QNetworkReply& reply)
{
    DEBUG("Get nzb data reply %1", reply);

    const auto err = reply.error();
    const auto url = reply.url();
    if (err != QNetworkReply::NoError)
    {
        ERROR("Failed to retrieve NZB content (%1), %2", url, err);
        return;
    }
    const auto buff = reply.readAll();
    DEBUG("NZB buffer %1 bytes", buff.size());

    cb(buff);
}



} // app
