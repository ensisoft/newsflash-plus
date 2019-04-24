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
#  include <QAbstractTableModel>
#  include <QDateTime>
#  include <QString>
#  include <QList>
#include "newsflash/warnpop.h"

#include <vector>
#include <map>

#include "engine/bitflag.h"
#include "media.h"

namespace app
{
    struct NewsGroupInfo;
    struct HeaderUpdateInfo;
    class Settings;

    class NewsList : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum class Columns {
            Messages,
            Category,
            SizeOnDisk,
            Favorite,
            Name,
            LAST
        };

        NewsList();
       ~NewsList();

        // QAbstractTableModel
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual QVariant data(const QModelIndex& index, int role) const override;
        virtual void sort(int column, Qt::SortOrder order) override;
        virtual int rowCount(const QModelIndex&) const override;
        virtual int columnCount(const QModelIndex&) const override;

        void clear();
        void stopRefresh(quint32 accountId);
        void makeListing(quint32 accountId);
        void loadListing(quint32 accountId);

        void markItemsFavorite(QModelIndexList& list);
        void clearFavoriteMarkFromItems(QModelIndexList& list);
        void toggleFavoriteMark(QModelIndexList& list);

        void clearSize(const QModelIndex& index);

        bool isUpdating(quint32 account) const;
        bool hasListing(quint32 account) const;

        bool hasData(const QModelIndex& index) const;
        bool isFavorite(const QModelIndex& index) const;
        QString getName(const QModelIndex& index) const;
        QString getName(std::size_t index) const;
        MediaType getMediaType(const QModelIndex& index) const;

        std::size_t numItems() const;

        enum class FilterFlags {
            ShowEmpty,
            ShowText,
            ShowMusic,
            ShowMovies,
            ShowTv,
            ShowGames,
            ShowApps,
            ShowAdult,
            ShowImages,
            ShowOther
        };

        void filter(const QString& str, newsflash::bitflag<FilterFlags> options);

    signals:
        void progressUpdated(quint32 acc, quint32 maxValue, quint32 curValue);
        void loadComplete(quint32 acc);
        void makeComplete(quint32 acc);
        void listUpdate(quint32 acc);

    private slots:
        void listCompleted(quint32 acc, const QList<app::NewsGroupInfo>& list);
        void listUpdated(quint32 acc, const QList<app::NewsGroupInfo>& list);
        void newHeaderDataAvailable(const app::HeaderUpdateInfo& info);

    private:
        void setAccountFavorites();

        enum Flags {
            Favorite = 0x1,          
        };

        struct NewsGroup {
            QString   name;
            quint64   numMessages = 0;
            quint64   sizeOnDisk  = 0;
            quint32   flags       = 0;
            MediaType type        = MediaType::Other;
        };
        struct RefreshListAction {
            quint32 account = 0;
            quint32 taskId  = 0;
            QString file;
            std::vector<NewsGroup> intermediateList;
        };

    private:
        quint32 mCurrentAccountId  = 0;
        std::vector<NewsGroup> mCurrentGroupList;
        std::size_t mNumCurrentlyVisible = 0;
    private:
        std::map<quint32, RefreshListAction> mPendingRefreshes;
    private:
    };

} // app
