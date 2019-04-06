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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtGui/QIcon>
#  include <QIODevice>
#  include <QDateTime>
#  include <QUrl>
#  include <QByteArray>
#  include <QBuffer>
#  include <QDir>
#include "newsflash/warnpop.h"

#include "rssreader.h"
#include "eventlog.h"
#include "debug.h"
#include "format.h"
#include "nzbparse.h"
#include "engine.h"
#include "platform.h"
#include "webquery.h"
#include "webengine.h"
#include "download.h"

namespace app
{

RSSReader::RSSReader() : mModel(new ModelType)
{
    DEBUG("RSSReader created");
}

RSSReader::~RSSReader()
{
    DEBUG("RSSReader deleted");
    stop();
}

bool RSSReader::refresh(MainMediaType type)
{
    std::vector<QUrl> urls;
    mEngine->prepare(type, urls);
    for (const auto& url : urls)
    {
        WebQuery query(url);
        query.OnReply = std::bind(&RSSReader::onRefreshComplete, this,
            mEngine.get(), std::placeholders::_1);
        auto* p = g_web->submit(query);
        mWebQueries.push_back(p);
    }
    mModel->clear();
    return !urls.empty();
}

QAbstractTableModel* RSSReader::getModel()
{
    return mModel.get();
}

void RSSReader::downloadNzbFile(std::size_t index, const QString& file)
{
    const auto& item = mModel->getItem(index);
    const auto& link = item.nzblink;

    WebQuery query(link);
    query.OnReply = std::bind(&RSSReader::onNzbFileComplete, this, file,
        std::placeholders::_1);
    auto* ret = g_web->submit(query);
    mWebQueries.push_back(ret);
}

void RSSReader::downloadNzbFile(std::size_t index, DataCallback cb)
{
    const auto& item = mModel->getItem(index);
    const auto& link = item.nzblink;

    WebQuery query(link);
    query.OnReply = std::bind(&RSSReader::onNzbDataCompleteCallback, this, std::move(cb),
        std::placeholders::_1);
    auto* ret = g_web->submit(query);
    mWebQueries.push_back(ret);
}

void RSSReader::downloadNzbContent(std::size_t index, quint32 account, const QString& folder)
{
    const auto& item = mModel->getItem(index);
    const auto& link = item.nzblink;
    const auto& desc = item.title;

    WebQuery query(link);
    query.OnReply = std::bind(&RSSReader::onNzbDataComplete, this, folder, desc, item.type, account,
        std::placeholders::_1);
    auto* ret = g_web->submit(query);
    mWebQueries.push_back(ret);
}

void RSSReader::stop()
{
    for (auto* query : mWebQueries)
        query->abort();

    mWebQueries.clear();
}

const MediaItem& RSSReader::getItem(std::size_t i) const
{
    return mModel->getItem(i);
}
const MediaItem& RSSReader::getItem(const QModelIndex& i) const
{
    return mModel->getItem(i.row());
}


bool RSSReader::isEmpty() const
{
    return mModel->isEmpty();
}

std::size_t RSSReader::numItems() const
{
    return mModel->numItems();
}

void RSSReader::onRefreshComplete(RSSFeed* feed, QNetworkReply& reply)
{
    DEBUG("RSS stream reply %1", reply);

    const auto it = std::find_if(std::begin(mWebQueries), std::end(mWebQueries),
        [&](const WebQuery* q ){
            return q->isOwner(reply);
        });
    ENDCHECK(mWebQueries, it);
    mWebQueries.erase(it);
    if (mWebQueries.empty())
        OnReadyCallback();

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
        i.pubdate = i.pubdate.toLocalTime();
    }

    mModel->append(std::move(items));
}

void RSSReader::onNzbFileComplete(const QString& file, QNetworkReply& reply)
{
    DEBUG("Got nzb data reply %1", reply);

    const auto it = std::find_if(std::begin(mWebQueries), std::end(mWebQueries),
        [&](const WebQuery* q ){
            return q->isOwner(reply);
        });
    ENDCHECK(mWebQueries, it);
    mWebQueries.erase(it);
    if (mWebQueries.empty())
        OnReadyCallback();

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

void RSSReader::onNzbDataComplete(const QString& folder, const QString& desc, MediaType type, quint32 acc, QNetworkReply& reply)
{
    DEBUG("Got nzb data reply %1", reply);

    const auto it = std::find_if(std::begin(mWebQueries), std::end(mWebQueries),
        [&](const WebQuery* q ){
            return q->isOwner(reply);
        });
    ENDCHECK(mWebQueries, it);
    mWebQueries.erase(it);
    if (mWebQueries.empty())
        OnReadyCallback();

    const auto err = reply.error();
    const auto url = reply.url();
    if (err != QNetworkReply::NoError)
    {
        ERROR("Failed to retrieve NZB content (%1), %2", url, err);
        return;
    }

    QByteArray nzb = reply.readAll();

    Download download;
    download.type     = type;
    download.source   = MediaSource::RSS;
    download.account  = acc;
    download.basepath = folder;
    download.folder   = desc;
    download.desc     = desc;
    g_engine->downloadNzbContents(download, nzb);
}

void RSSReader::onNzbDataCompleteCallback(const DataCallback& cb, QNetworkReply& reply)
{
    DEBUG("Got nzb data reply %1", reply);

    const auto it = std::find_if(std::begin(mWebQueries), std::end(mWebQueries),
        [&](const WebQuery* q ){
            return q->isOwner(reply);
        });
    ENDCHECK(mWebQueries, it);
    mWebQueries.erase(it);
    if (mWebQueries.empty())
        OnReadyCallback();

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
