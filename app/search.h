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
#  include <QtNetwork/QNetworkReply>
#  include <QString>
#include <newsflash/warnpop.h>
#include <functional>
#include <memory>
#include "media.h"
#include "searchmodel.h"
#include "tablemodel.h"
#include "netman.h"

class QAbstractTableModel;

namespace app
{
    class Indexer;

    class Search
    {
    public:
        struct Basic {
            QString keywords;
        };

        struct Advanced {
            QString keywords;
            bool music;
            bool movies;
            bool television;
            bool console;
            bool computer;
            bool porno;

        };

        struct Music {
            QString album;
            QString track;
            QString year;
            QString keywords;
        };

        struct Television {
            QString season;
            QString episode;
            QString keywords;
        };

        Search();
       ~Search();

        using OnReady = std::function<void ()>;
        using OnData  = std::function<void (const QByteArray& bytes, const QString& desc)>;

        OnReady OnReadyCallback;

        QAbstractTableModel* getModel();

        // begin searching with the given criteria.
        bool beginSearch(const Basic& query, std::unique_ptr<Indexer> index);
        bool beginSearch(const Advanced& query, std::unique_ptr<Indexer> index);
        bool beginSearch(const Music& query, std::unique_ptr<Indexer> index);
        bool beginSearch(const Television& query, std::unique_ptr<Indexer> index);

        // stop current actions.
        void stop();

        // get the item contents of the selected items and invoke
        // the callback for the content buffer.
        void loadItem(const QModelIndex& index, OnData cb);

        // get the item contents and save them to the specified file.
        void saveItem(const QModelIndex& index, const QString& file);

        const MediaItem& getItem(const QModelIndex& index) const;
    private:
        void onSearchReady(QNetworkReply& reply);

    private:
        using ModelType = TableModel<SearchModel, MediaItem>;

        std::unique_ptr<ModelType> model_;
        std::unique_ptr<Indexer> indexer_;
        NetworkManager::Context net_;
    };

} // app