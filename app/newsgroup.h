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
#include <newsflash/warnpop.h>
#include <newsflash/engine/filebuf.h>
#include <newsflash/engine/filemap.h>
#include <newsflash/engine/catalog.h>
#include <newsflash/engine/index.h>
#include <vector>
#include <deque>
#include "filetype.h"

namespace app
{
    struct HeaderInfo;

    class NewsGroup : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum class Columns {
            Type,
            Age, 
            BrokenFlag,            
            Size,  
            //Author,  
            DownloadFlag, 
            BookmarkFlag,            
            Subject, LAST
        };

        NewsGroup();
       ~NewsGroup();

        // QAbstractTableModel
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual QVariant data(const QModelIndex& index, int role) const override;        
        virtual int rowCount(const QModelIndex&) const override;
        virtual int columnCount(const QModelIndex&) const override;
        virtual void sort(int column, Qt::SortOrder order) override;

        bool load(quint32 blockIndex, QString path, QString name);
        void refresh(quint32 account, QString path, QString name);
        void stop();

        std::size_t numItems() const;

    public slots:
        void newHeaderDataAvailable(const QString& file);
        void newHeaderInfoAvailable(const QString& group, quint64 numLocal, quint64 numRemote);
        void updateCompleted(const app::HeaderInfo& info);

    private:
        using filedb = newsflash::catalog<newsflash::filemap>;
        using index  = newsflash::index;
        std::vector<filedb> filedbs_;
        std::vector<std::size_t> offsets_;
        index index_;

    private:
        quint32 task_;        
        QString path_;
        QString name_;

    };
} // app