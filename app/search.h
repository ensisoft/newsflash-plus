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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include "QtNetwork/QNetworkReply"
#  include <QString>
#include "newsflash/warnpop.h"

#include <functional>
#include <memory>
#include <list>

#include "media.h"
#include "searchmodel.h"
#include "tablemodel.h"

class QAbstractTableModel;

namespace app
{
    class Indexer;
    class WebQuery;

    // Search class can perform queries against a Usenet indexer.
    class Search
    {
    public:
        // basic search
        struct Basic {
            QString keywords;
            quint32 qoffset = 0;
            quint32 qsize   = 0;
        };

        // advanced search allows us to limit the scope
        // of the search to some specific media streams.
        struct Advanced {
            QString keywords;
            bool music = false;
            bool movies = false;
            bool television = false;
            bool console = false;
            bool computer = false;
            bool adult = false;
            quint32 qoffset = 0;
            quint32 qsize   = 0;
        };

        // music search optionally searching for specific
        // track/album/year
        struct Music {
            QString album;
            QString track;
            QString year;
            QString keywords;
            quint32 qoffset = 0;
            quint32 qsize   = 0;
        };

        // television search optionally searching
        // for specific episode/season.
        struct Television {
            QString season;
            QString episode;
            QString keywords;
            quint32 qoffset = 0;
            quint32 qsize   = 0;
        };

        Search();
       ~Search();

        using OnReady  = std::function<void ()>;
        using OnData   = std::function<void (const QByteArray& bytes, const QString& desc)>;
        using OnSearch = std::function<void (bool empty)>;

        OnReady  OnReadyCallback;
        OnSearch OnSearchCallback;

        QAbstractTableModel* getModel();

        // begin searching with the given criteria.
        bool beginSearch(const Basic& query, std::unique_ptr<Indexer> index);
        bool beginSearch(const Advanced& query, std::unique_ptr<Indexer> index);
        bool beginSearch(const Music& query, std::unique_ptr<Indexer> index);
        bool beginSearch(const Television& query, std::unique_ptr<Indexer> index);

        // stop current actions.
        void stop();

        void clear();

        // get the item contents of the selected items and invoke
        // the callback for the content buffer.
        void loadItem(const QModelIndex& index, OnData cb);

        // get the item contents and save them to the specified file.
        void saveItem(const QModelIndex& index, const QString& file);

        void downloadItem(const QModelIndex& index,
            const QString& folder, quint32 account);

        const MediaItem& getItem(const QModelIndex& index) const;

    private:
        void doSearch(QUrl url);
        void onSearchReady(QNetworkReply& reply);

    private:
        using ModelType = TableModel<SearchModel, MediaItem>;
        std::unique_ptr<ModelType> mModel;
        std::unique_ptr<Indexer> mIndexer;
        std::list<WebQuery*> mWebQueries;
    };

} // app
