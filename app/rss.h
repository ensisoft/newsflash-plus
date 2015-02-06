// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

    class RSS : public QAbstractTableModel
    {
    public:
        RSS();
       ~RSS();
        
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
        bool refresh(Media m);

        void enableFeed(const QString& feed, bool on_off);

        void setCredentials(const QString& feed, const QString& user, const QString& apikey);

        using data_callback = std::function<void (const QByteArray&)>;

        // save NZB description of the RSS item and place the .nzb file in the given folder.
        void downloadNzbFile(int row, const QString& file);

        // download the contents of the RSS item and upon completion
        // invoke the callback handler.
        void downloadNzbFile(int row, data_callback cb);

        // download the contents of the RSS item.
        void downloadNzbContent(int row, quint32 account, const QString& folder);

        // stop/cancel pending network operations.
        void stop();

        const MediaItem& getItem(std::size_t i) const
        { return items_[i]; }

        bool isEmpty() const
        { return items_.empty(); }

    private:
        void onRefreshComplete(RSSFeed* feed, Media type, QNetworkReply& reply);
        void onNzbFileComplete(const QString& file, QNetworkReply& reply);
        void onNzbDataComplete(const QString& folder, const QString& title, quint32 acc, QNetworkReply& rely);
        void onNzbDataCompleteCallback(const data_callback& cb, QNetworkReply& reply);

    private:
        enum class columns {
            date, category, size, title, sentinel
        };                

    private:
        std::vector<std::unique_ptr<RSSFeed>> feeds_;
        std::vector<MediaItem> items_;          
    private:
        NetworkManager::Context net_;
    };

} // rss
