// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

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

#include "rss.h"
#include "eventlog.h"
#include "debug.h"
#include "format.h"
#include "nzbparse.h"
#include "womble.h"
#include "nzbs.h"

namespace app
{

rss::rss()
{
    feeds_.emplace_back(new womble);
    feeds_.emplace_back(new nzbs);
    INFO("RSS http://nzbs.org");
    INFO("RSS http://newshost.co.za");

    net_.on_ready = [=]() {
        on_ready();
    };

    DEBUG("rss app created");
}

rss::~rss()
{
    DEBUG("rss app destroyed");
}

QVariant rss::data(const QModelIndex& index, int role) const
{
    const auto row = index.row();
    const auto col = columns(index.column());        
    const auto& item = items_[row];

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
            case columns::date:
                return format(app::event {item.pubdate}); 

            case columns::category:
                return str(item.type);

            case columns::title:
                return item.title;

            case columns::size:
                if (item.size == 0)
                    return "n/a";
                return format(size { item.size });

            default:
                Q_ASSERT(!"unknown column");
                break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        if (col  == columns::date)
            return QIcon(":/resource/16x16_ico_png/ico_rss.png");
        else if (col == columns::title && item.password)
            return QIcon(":/resource/16x16_ico_png/ico_password.png");
    }
    return QVariant();
}

QVariant rss::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (columns(section))
    {
        case columns::date:     return "Date";
        case columns::category: return "Category";
        case columns::size:     return "Size";
        case columns::title:    return "Title";
        default:
        Q_ASSERT(!"unknown column");
        break;
    }
    return QVariant();
}

void rss::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();

    #define SORT(x) \
        std::sort(std::begin(items_), std::end(items_), \
            [&](const mediaitem& lhs, const mediaitem& rhs) { \
                if (order == Qt::AscendingOrder) \
                    return lhs.x < rhs.x; \
                return lhs.x > rhs.x; \
            });
        switch (columns(column))
        {
            case columns::date:     SORT(pubdate); break;
            //case columns::category: SORT(tags);    break;
            case columns::size:     SORT(size);    break;
            case columns::title:    SORT(title);   break;
            default:
            Q_ASSERT(!"unknown column");
            break;
        }
    #undef SORT

    emit layoutChanged();
}

int rss::rowCount(const QModelIndex&) const
{
    return (int)items_.size();
}

int rss::columnCount(const QModelIndex&) const
{
    return (int)columns::sentinel;
}


void rss::clear()
{
    items_.clear();
    QAbstractTableModel::reset();                
}

bool rss::refresh(media type)
{
    bool ret = false;

    for (auto& feed : feeds_)
    {
        if (!feed->is_enabled())
            continue;

        std::vector<QUrl> urls;
        feed->prepare(type, urls);
        for (const auto& url : urls)
        {
            net_.submit(std::bind(&rss::on_refresh_complete, this, feed.get(), type,
                std::placeholders::_1), url);
            ret = true;
        }
    }

    return ret;
}

void rss::enable_feed(const QString& feed, bool on_off)
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

void rss::set_credentials(const QString& feed, const QString& user, const QString& apikey)
{
    rssfeed::params p;
    p.user = user;
    p.key  = apikey;
    for (auto& f : feeds_)
    {
        if (f->name() == feed)
        {
            f->set_params(p);
            return;
        }
    }
}


void rss::download_nzb_file(int row, const QString& folder)
{
    Q_ASSERT(row >= 0);
    Q_ASSERT(row < items_.size());

    const auto& item = items_[row];
    const auto& link = item.nzblink;
    const auto& name = item.title;
    const auto& file = QDir::toNativeSeparators(folder + "/" + name + ".nzb");
}

void rss::download_nzb_content(int row, const QString& folder)
{
    Q_ASSERT(row >= 0);
    Q_ASSERT(row < items_.size());    
}

void rss::stop()
{
    net_.cancel();
}

void rss::on_refresh_complete(rssfeed* feed, media type, QNetworkReply& reply)
{
    const auto err = reply.error();
    const auto url = reply.url();
    if (reply.error())
    {
        ERROR(str("RSS refresh failed (_1), _2)", url, str(err)));
        return;
    }
    QByteArray bytes = reply.readAll();
    QBuffer io(&bytes);

    //qDebug() << bytes.size();
    //qDebug() << bytes;

    std::vector<mediaitem> items;

    if (!feed->parse(io, items))
    {
        ERROR(str("RSS XML parse failed (_1)", url));
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

    INFO(str("RSS feed from _1 complete", url));
}

} // app
