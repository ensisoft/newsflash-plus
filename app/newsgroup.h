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
#  include <QObject>
#  include <QDateTime>
#include "newsflash/warnpop.h"

#include <vector>
#include <deque>
#include <memory>
#include <functional>
#include <ctime>

#include "engine/filebuf.h"
#include "engine/filemap.h"
#include "engine/catalog.h"
#include "engine/index.h"
#include "engine/idlist.h"
#include "engine/bitflag.h"
#include "stringlib/string.h"
#include "filetype.h"
#include "debug.h"
#include "format.h"
#include "types.h"
#include "media.h"

//#include <valgrind/callgrind.h>

class QEventLoop;

namespace app
{
    struct HeaderInfo;
    struct HeaderUpdateInfo;

    class NewsGroup : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum class Columns {
            Type,
            DownloadFlag,
            Age,
            BrokenFlag,
            Size,
            //Author,
            BookmarkFlag,
            Subject, LAST
        };

        NewsGroup();
       ~NewsGroup();

        std::function<void (std::size_t curBlock, std::size_t numBlocks)> onLoadBegin;
        std::function<void (std::size_t curItem, std::size_t numMitems)> onLoadProgress;
        std::function<void (std::size_t curBlock, std::size_t numBlocks)> onLoadComplete;
        std::function<void ()> onKilled;

        // QAbstractTableModel
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual QVariant data(const QModelIndex& index, int role) const override;
        virtual int rowCount(const QModelIndex&) const override;
        virtual int columnCount(const QModelIndex&) const override;
        virtual void sort(int column, Qt::SortOrder order) override;

        bool init(QString path, QString name);
        void load(std::size_t blockIndex);
        void load();
        void purge(std::size_t blockIndex);
        void refresh(quint32 account, QString path, QString name);
        void stop();

        void killSelected(const QModelIndexList& list);
        void scanSelected(QModelIndexList& list);
        void select(const QModelIndexList& list, bool val);

        void bookmark(const QModelIndexList& list);

        void download(const QModelIndexList& list, quint32 acc, const QString& folder);

        // sum the sizes of the selected items.
        quint64 sumDataSizes(const QModelIndexList& list) const;

        // suggest a batch name for the selected items.
        QString suggestName(const QModelIndexList& list) const;

        std::size_t numItems() const;
        std::size_t numShown() const;
        std::size_t numBlocksAvail() const;
        std::size_t numBlocksLoaded() const;

        using Article  = newsflash::article<newsflash::filemap>;
        using FileType = newsflash::filetype;
        using FileFlag = newsflash::fileflag;

        Article getArticle(std::size_t index) const;

        void setTypeFilter(FileType type, bool onOff)
        {
            mShowTheseFileTypes.set(type, onOff);
        }
        void setFlagFilter(FileFlag flag, bool onOff)
        {
            mShowTheseFileFlags.set(flag, onOff);
        }

        void setSizeFilter(quint64 minSize, quint64 maxSize)
        {
            DEBUG("Size filter %1 - %2", app::size{minSize}, app::size{maxSize});

            mMinShowFileSize = minSize;
            mMaxShowFileSize = maxSize;
        }

        void setDateFilter(quint32 minDays, quint32 maxDays)
        {
            const auto now = QDateTime::currentDateTime();
            const auto beg = now.addDays(-(int)minDays);
            const auto end = now.addDays(-(int)maxDays);

            DEBUG("Date filter %1 - %2", app::age{beg}, app::age{end});

            mMinShowPubdate = end.toTime_t();
            mMaxShowPubdate = beg.toTime_t();
        }

        void setStringFilter(const QString& str, bool case_sensitive)
        {
            mFilterString = str;
            mFilterStringUtf8.reset(toUtf8(str), case_sensitive);
            mIsFilterStringCaseSensitive = case_sensitive;
        }

        void applyFilter()
        {
            //CALLGRIND_START_INSTRUMENTATION;
            //CALLGRIND_ZERO_STATS;

            QAbstractTableModel::beginResetModel();
            mIndex.filter();
            QAbstractTableModel::endResetModel();

            //CALLGRIND_DUMP_STATS_AT("applyFilter");
            //CALLGRIND_STOP_INSTRUMENTATION;
        }

        QAbstractTableModel* getVolumeList();

        MediaType findMediaType() const
        {
            return app::findMediaType(mGroupName);
        }

        static
        void deleteData(quint32 account, QString path, QString group);

    public slots:
        void newHeaderDataAvailable(const app::HeaderUpdateInfo& info);
        void updateCompleted(const app::HeaderInfo& info);
        void actionKilled(quint32 action);

    private:
        struct Block;
        void loadData(Block& block, bool guiLoad, const void* snapshot = nullptr);

        bool showDeleted() const {
            return mShowTheseFileFlags.test(newsflash::fileflag::deleted);
        }

        using catalog = newsflash::catalog<newsflash::filemap>;
        using index   = newsflash::index<newsflash::filemap>;
        using idlist  = newsflash::idlist<newsflash::filebuf>;

        enum class State {
            Loaded, UnLoaded
        };

        struct Block {
            std::size_t prevSize   = 0;
            std::size_t prevOffset = 0;
            std::size_t index = 0;
            QString file;
            State state = State::UnLoaded;
            bool purge  = false;
        };

        class VolumeList;
        std::unique_ptr<VolumeList> mVolumeList;

        std::deque<Block> mLoadedBlocks;
        std::deque<catalog> mCatalogs;
        index mIndex;
        idlist mArticleNumberList;

    private:
        quint32 mNumCurrentlySelectedItems = 0;
        quint32 mCurrentUpdateTaskId = 0;
        QString mGroupPath;
        QString mGroupName;
    private:
        // filtering options
        newsflash::bitflag<FileType, std::uint16_t> mShowTheseFileTypes;
        newsflash::bitflag<FileFlag, std::uint16_t> mShowTheseFileFlags;
        std::uint64_t mMinShowFileSize = 0;
        std::uint64_t mMaxShowFileSize = 0;
        std::time_t   mMinShowPubdate  = 0;
        std::time_t   mMaxShowPubdate  = 0;

        QString mFilterString;
        bool mIsFilterStringCaseSensitive = false;
        str::string_matcher<> mFilterStringUtf8;

    };
} // app