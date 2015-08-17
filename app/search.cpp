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

#define LOGTAG "search"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkReply>
#  include <QBuffer>
#include <newsflash/warnpop.h>
#include <functional>
#include "search.h"
#include "debug.h"
#include "indexer.h"
#include "eventlog.h"
#include "webquery.h"
#include "webengine.h"
#include "download.h"
#include "engine.h"

namespace app
{ 

Search::Search() : model_(new ModelType)
{
    DEBUG("Search created");
}

Search::~Search()
{
    DEBUG("Search deleted");
}

QAbstractTableModel* Search::getModel()
{ return model_.get(); }

bool Search::beginSearch(const Basic& query, std::unique_ptr<Indexer> index)
{
    Indexer::BasicQuery q;
    q.keywords  = query.keywords;
    q.offset    = query.qoffset;
    q.size      = query.qsize;

    QUrl url;
    indexer_ = std::move(index);        
    indexer_->prepare(q, url);

    doSearch(url);

    return true;
}

bool Search::beginSearch(const Advanced& query, std::unique_ptr<Indexer> index)
{
    using c = Indexer::Category;
    using v = Indexer::Categories;
    v bits;
    bits.set(c::Music, query.music);
    bits.set(c::Movies, query.movies);
    bits.set(c::Television, query.television);
    bits.set(c::Console, query.console);
    bits.set(c::Apps, query.computer);
    bits.set(c::Adult, query.adult);

    Indexer::AdvancedQuery q;
    q.categories = bits;
    q.keywords   = query.keywords;
    q.offset     = query.qoffset;
    q.size       = query.qsize;

    QUrl url;
    indexer_ = std::move(index);
    indexer_->prepare(q, url);

    doSearch(url);

    return true;
}

bool Search::beginSearch(const Music& query, std::unique_ptr<Indexer> index)
{
    Indexer::MusicQuery q;
    q.artist = query.keywords;
    q.album  = query.album;
    q.track  = query.track;
    q.year   = query.year;
    q.offset = query.qoffset;
    q.size   = query.qsize;

    QUrl url;
    indexer_ = std::move(index);
    indexer_->prepare(q, url);

    doSearch(url);

    return true;
}

bool Search::beginSearch(const Television& query, std::unique_ptr<Indexer> index)
{
    Indexer::TelevisionQuery q;
    q.episode  = query.episode;
    q.season   = query.season;
    q.keywords = query.keywords;
    q.offset   = query.qoffset;
    q.size     = query.qsize;

    QUrl url;
    indexer_ = std::move(index);
    indexer_->prepare(q, url);

    doSearch(url);

    return true;
}

void Search::stop()
{
    for (auto* query : queries_)
        query->abort();

    queries_.clear();
}

void Search::clear()
{
    model_->clear();
}

void Search::loadItem(const QModelIndex& index, OnData cb)
{
    const auto& item = model_->getItem(index);
    const auto& link = item.nzblink;
    const auto& desc = item.title;

    WebQuery query(link);
    query.OnReply = [=](QNetworkReply& reply) 
    {
        const auto it = std::find_if(std::begin(queries_), std::end(queries_),
            [&](const WebQuery* q ){
                return q->isOwner(reply);
            });
        ENDCHECK(queries_, it);
        queries_.erase(it);
        if (queries_.empty())
            OnReadyCallback();        

        const auto err = reply.error();
        const auto url = reply.url();
        if (err != QNetworkReply::NoError)
        {
            ERROR("Failed to retrieve NZB content (%1), %2", url, err);
            return;
        }
        auto buff = reply.readAll();

        cb(buff, desc);
    };
    auto* ret = g_web->submit(query);
    queries_.push_back(ret);
}

void Search::saveItem(const QModelIndex& index, const QString& file) 
{
    const auto& item = getItem(index);
    const auto& link = item.nzblink;

    WebQuery query(link);
    query.OnReply = [=](QNetworkReply& reply) 
    {
        const auto it = std::find_if(std::begin(queries_), std::end(queries_),
            [&](const WebQuery* q ){
                return q->isOwner(reply);
            });
        ENDCHECK(queries_, it);
        queries_.erase(it);
        if (queries_.empty())
            OnReadyCallback();        

        const auto err = reply.error();
        const auto url = reply.url();
        if (err != QNetworkReply::NoError)
        {
            ERROR("Failed to retrieve NZB content (%1), %2", url, err);
            return;
        }
        QFile io(file);
        if (!io.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            ERROR("Failed to open file %1 for writing %2", file, io.error());
            return;
        }
        auto buff = reply.readAll();
        io.write(buff);
        io.close();
        INFO("Saved NZB file %1 %2", file, size {(unsigned)buff.size()});

    };
    auto* ret = g_web->submit(query);
    queries_.push_back(ret);
}

void Search::downloadItem(const QModelIndex& index, const QString& folder, quint32 account)
{
    const auto& item = getItem(index);
    const auto& link = item.nzblink;

    WebQuery query(link);
    query.OnReply = [=](QNetworkReply& reply)
    {
        const auto it = std::find_if(std::begin(queries_), std::end(queries_),
            [&](const WebQuery* q) {
                return q->isOwner(reply);
            });
        ENDCHECK(queries_, it);
        queries_.erase(it);
        if (queries_.empty())
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
        download.type     = item.type;
        download.source   = MediaSource::Search;
        download.account  = account;
        download.basepath = folder;
        download.folder   = item.title;
        download.desc     = item.title;
        g_engine->downloadNzbContents(download, nzb);

    };
    auto* ret = g_web->submit(query);
    queries_.push_back(ret);
}

const MediaItem& Search::getItem(const QModelIndex& index) const 
{
    return model_->getItem(index);
}

void Search::doSearch(QUrl url)
{
    WebQuery web(url);
    web.OnReply = std::bind(&Search::onSearchReady, this,
        std::placeholders::_1);
    WebQuery* ret = g_web->submit(web);
    queries_.push_back(ret);
}

void Search::onSearchReady(QNetworkReply& reply)
{
    const auto it = std::find_if(std::begin(queries_), std::end(queries_),
        [&](const WebQuery* q ){
            return q->isOwner(reply);
        });
    ENDCHECK(queries_, it);
    queries_.erase(it);
    if (queries_.empty())
        OnReadyCallback();    

    const auto err = reply.error();
    const auto url = reply.url();
    if (err != QNetworkReply::NoError)
    {
        ERROR("Search failed (%1), %2", url, err);
        return;
    }

    auto buff = reply.readAll();
    QBuffer io(&buff);

    std::vector<MediaItem> items;

    using e = Indexer::Error;

    const auto error = indexer_->parse(io, items);
    const auto name  = indexer_->name();
    switch (error)
    {
        case e::None: break;
        case e::Content:
            ERROR("%1 Content error.", name);
            break;
        case e::NoSuchItem: 
            ERROR("%1, No such item.", name);
            break;
        case e::IncorrectCredentials:
            ERROR("%1, Incorrect credentials.", name);
            break;
        case e::AccountSuspended:
            ERROR("%1, Account is suspended.", name);
            break;
        case e::NoPermission:
            ERROR("%1 No permission.", name);
            break;
        case e::Unknown:
            ERROR("%1 Unknown error.", name);
            break;
    }

    const auto emptyResult = items.empty();

    model_->append(std::move(items));

    OnSearchCallback(emptyResult);
}

} // app
