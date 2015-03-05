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
#include <functional>
#include "media.h"
#include "netman.h"
#include "rssfeed.h"

namespace app
{
    class RSSFeed;

    // RSSReader aggregates several RSS feeds into one single RSS feed.
    class RSSReader : public QAbstractTableModel
    {
    public:
        enum class columns {
            date, category, size, locked, title, sentinel
        };                

        RSSReader();
       ~RSSReader();
        
        std::function<void()> on_ready;

        // QAbstractTableModel
        virtual QVariant data(const QModelIndex& index, int role) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual void sort(int column, Qt::SortOrder order) override;
        virtual int rowCount(const QModelIndex&) const  override;
        virtual int columnCount(const QModelIndex&) const override;

        // clear the data model and remove all RSS feed items.
        void clear();

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

        const MediaItem& getItem(std::size_t i) const
        { return items_[i]; }

        bool isEmpty() const
        { return items_.empty(); }

    private:
        void onRefreshComplete(RSSFeed* feed, MediaType type, QNetworkReply& reply);
        void onNzbFileComplete(const QString& file, QNetworkReply& reply);
        void onNzbDataComplete(const QString& folder, const QString& title, quint32 acc, QNetworkReply& rely);
        void onNzbDataCompleteCallback(const data_callback& cb, QNetworkReply& reply);

    private:
        std::vector<std::unique_ptr<RSSFeed>> feeds_;
        std::vector<MediaItem> items_;          
    private:
        NetworkManager::Context net_;
    };

} // rss
