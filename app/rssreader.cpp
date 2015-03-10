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
#include "webquery.h"
#include "webengine.h"

namespace app
{

RSSReader::RSSReader() : model_(new ModelType)
{
    feeds_.emplace_back(new Womble);
    feeds_.emplace_back(new Nzbs);

    // net_.callback = [=] {
    //     on_ready();
    // };

    DEBUG("RSSReader created");
}

RSSReader::~RSSReader()
{
    DEBUG("RSSReader destroyed");
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
            WebQuery query(url);
            query.OnReply = std::bind(&RSSReader::onRefreshComplete, this, 
                feed.get(), type, std::placeholders::_1);
            auto* p = g_web->submit(query);
            queries_.push_back(p);
            ret = true;
        }
    }
    model_->clear();
    return ret;
}

QAbstractTableModel* RSSReader::getModel()
{
    return model_.get();
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
    const auto& item = model_->getItem(index);
    const auto& link = item.nzblink;

    WebQuery query(link);
    query.OnReply = std::bind(&RSSReader::onNzbFileComplete, this, file,
        std::placeholders::_1);
    auto* ret = g_web->submit(query);
    queries_.push_back(ret);
}

void RSSReader::downloadNzbFile(std::size_t index, data_callback cb)
{
    const auto& item = model_->getItem(index);
    const auto& link = item.nzblink;

    WebQuery query(link);
    query.OnReply = std::bind(&RSSReader::onNzbDataCompleteCallback, this, std::move(cb), 
        std::placeholders::_1);
    auto* ret = g_web->submit(query);
    queries_.push_back(ret);
}

void RSSReader::downloadNzbContent(std::size_t index, quint32 account, const QString& folder)
{
    const auto& item = model_->getItem(index);
    const auto& link = item.nzblink;
    const auto& desc = item.title;

    WebQuery query(link);
    query.OnReply = std::bind(&RSSReader::onNzbDataComplete, this, folder, desc, account,
        std::placeholders::_1);
    auto* ret = g_web->submit(query);
    queries_.push_back(ret);
}

void RSSReader::stop()
{
    for (auto* query : queries_)
        query->abort();

    queries_.clear();
}

const MediaItem& RSSReader::getItem(std::size_t i) const
{
    return model_->getItem(i);
}

bool RSSReader::isEmpty() const 
{
    return model_->isEmpty();
}

std::size_t RSSReader::numItems() const 
{
    return model_->numItems();
}

void RSSReader::onRefreshComplete(RSSFeed* feed, MediaType type, QNetworkReply& reply)
{
    DEBUG("RSS stream reply %1", reply);

    const auto it = std::find_if(std::begin(queries_), std::end(queries_),
        [&](const WebQuery* q ){
            return q->isOwner(reply);
        });
    ENDCHECK(queries_, it);
    queries_.erase(it);
    if (queries_.empty())
        on_ready();

    const auto err = reply.error();
    const auto url = reply.url();
    if (err != QNetworkReply::NoError)
    {
        ERROR("RSS refresh failed (%1), %2", url, err);
        return;
    }
    QByteArray bytes = reply.readAll();
    QBuffer io(&bytes);

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

    model_->append(std::move(items));
}

void RSSReader::onNzbFileComplete(const QString& file, QNetworkReply& reply)
{
    DEBUG("Get nzb data reply %1", reply);

    const auto it = std::find_if(std::begin(queries_), std::end(queries_),
        [&](const WebQuery* q ){
            return q->isOwner(reply);
        });
    ENDCHECK(queries_, it);
    queries_.erase(it);
    if (queries_.empty())
        on_ready();

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

    const auto it = std::find_if(std::begin(queries_), std::end(queries_),
        [&](const WebQuery* q ){
            return q->isOwner(reply);
        });
    ENDCHECK(queries_, it);
    queries_.erase(it);
    if (queries_.empty())
        on_ready();

    const auto err = reply.error();
    const auto url = reply.url();
    if (err != QNetworkReply::NoError)
    {
        ERROR("Failed to retrieve NZB content (%1), %2", url, err);
        return;
    }

    const auto bytes = reply.readAll();

    g_engine->downloadNzbContents(acc, folder, desc, bytes);
}

void RSSReader::onNzbDataCompleteCallback(const data_callback& cb, QNetworkReply& reply)
{
    DEBUG("Get nzb data reply %1", reply);

    const auto it = std::find_if(std::begin(queries_), std::end(queries_),
        [&](const WebQuery* q ){
            return q->isOwner(reply);
        });
    ENDCHECK(queries_, it);
    queries_.erase(it);
    if (queries_.empty())
        on_ready();

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
