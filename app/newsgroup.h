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
#  include <QAbstractTableModel>
#  include <QObject>
#  include <QDateTime>
#include <newsflash/warnpop.h>
#include <newsflash/engine/filebuf.h>
#include <newsflash/engine/filemap.h>
#include <newsflash/engine/catalog.h>
#include <newsflash/engine/index.h>
#include <newsflash/engine/idlist.h>
#include <vector>
#include <deque>
#include <memory>
#include <functional>
#include "filetype.h"
#include "debug.h"
#include "format.h"
#include "types.h"

//#include <valgrind/callgrind.h>

class QEventLoop;

namespace app
{
    struct HeaderInfo;

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

        void download(const QModelIndexList& list, quint32 acc, QString folder);

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
            index_.set_type_filter(type, onOff);
        }
        void setFlagFilter(FileFlag flag, bool onOff)
        {
            index_.set_flag_filter(flag, onOff);
        }

        void setSizeFilter(quint64 minSize, quint64 maxSize)
        {
            DEBUG("Size filter %1 - %2", app::size{minSize}, app::size{maxSize});

            index_.set_size_filter(minSize, maxSize);
        }

        void setDateFilter(quint32 minDays, quint32 maxDays)
        {
            const auto now = QDateTime::currentDateTime();
            const auto beg = now.addDays(-(int)minDays);
            const auto end = now.addDays(-(int)maxDays);

            DEBUG("Date filter %1 - %2", app::age{beg}, app::age{end});

            index_.set_date_filter(end.toTime_t(), beg.toTime_t());
        }

        void applyFilter()
        {
            //CALLGRIND_START_INSTRUMENTATION;
            //CALLGRIND_ZERO_STATS;

            const auto curSize = index_.size();
            index_.filter();
            const auto newSize = index_.size();
            if (newSize != curSize)
                QAbstractTableModel::reset();

            //CALLGRIND_DUMP_STATS_AT("applyFilter");
            //CALLGRIND_STOP_INSTRUMENTATION;
        }

        QAbstractTableModel* getVolumeList();

        static 
        void deleteData(quint32 account, QString path, QString group);

    public slots:
        void newHeaderDataAvailable(const QString& file);
        void newHeaderInfoAvailable(const QString& group, quint64 numLocal, quint64 numRemote);
        void updateCompleted(const app::HeaderInfo& info);

    private:
        struct Block;
        void loadData(Block& block, bool guiLoad);

        using catalog = newsflash::catalog<newsflash::filemap>;
        using index   = newsflash::index<newsflash::filemap>;
        using idlist  = newsflash::idlist<newsflash::filemap>;

        enum class State {
            Loaded, UnLoaded
        };

        struct Block {
            std::size_t prevSize;
            std::size_t prevOffset;
            std::size_t index;
            QString file;
            State state;
            bool purge;
        };

        class VolumeList;
        std::unique_ptr<VolumeList> volumeList_;

        std::deque<Block> blocks_;
        std::deque<catalog> catalogs_;
        index index_;
        idlist idlist_;

    private:
        quint32 numSelected_;        
        quint32 task_;        
        QString path_;
        QString name_;

    };
} // app