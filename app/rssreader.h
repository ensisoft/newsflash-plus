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

#pragma once

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QString>
#  include <QObject>
#  include <QAbstractTableModel>
#  include <QDateTime>
#include <newsflash/warnpop.h>
#include <memory>
#include <vector>
#include <map>
#include <list>
#include <functional>
#include "media.h"
#include "rssfeed.h"
#include "rssmodel.h"
#include "tablemodel.h"

namespace app
{
    class RSSFeed;
    class WebQuery;

    // RSSReader aggregates several RSS feeds into one single RSS feed.
    class RSSReader
    {
    public:
        RSSReader();
       ~RSSReader();
        
        std::function<void()> on_ready;

        QAbstractTableModel* getModel();

        // refresh the RSS stream for media type m.
        // returns true if there are RSS feeds in this media
        // category to be refreshed. otherwise false and no network activity occurs.
        bool refresh(MediaType m);

        void enableFeed(const QString& feed, bool on_off);

        void setCredentials(const QString& feed, const QString& user, const QString& apikey);

        using data_callback = std::function<void (const QByteArray&)>;

        // save NZB description of the RSS item and place the .nzb file in the given folder.
        void downloadNzbFile(std::size_t index, const QString& file);

        // download the contents of the RSS item and upon completion
        // invoke the callback handler.
        void downloadNzbFile(std::size_t index, data_callback cb);

        // download the contents of the RSS item.
        void downloadNzbContent(std::size_t row, quint32 account, const QString& folder);

        // stop/cancel pending network operations.
        void stop();

        const MediaItem& getItem(std::size_t i) const;
        const MediaItem& getItem(const QModelIndex& i) const;

        bool isEmpty() const;

        std::size_t numItems() const;

    private:
        void onRefreshComplete(RSSFeed* feed, MediaType type, QNetworkReply& reply);
        void onNzbFileComplete(const QString& file, QNetworkReply& reply);
        void onNzbDataComplete(const QString& folder, const QString& title, MediaType type, quint32 acc, QNetworkReply& rely);
        void onNzbDataCompleteCallback(const data_callback& cb, QNetworkReply& reply);

    private:
        using ModelType = TableModel<RSSModel, MediaItem>;
        std::vector<std::unique_ptr<RSSFeed>> feeds_;
        std::unique_ptr<ModelType> model_;
        std::list<WebQuery*> queries_;
    };

} // rss
