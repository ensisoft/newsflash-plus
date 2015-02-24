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
#  include <QtGui/QIcon>
#  include <QObject>
#  include <QAbstractTableModel>
#  include <QString>
#  include <QDateTime>
#  include <QFile>
#include <newsflash/warnpop.h>
#include <vector>
#include "filetype.h"

namespace app
{
    struct DataFileInfo;

    class Files : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        struct File {
            QString   name;
            QString   path;
            QDateTime time;
            QIcon     icon;
            FileType  type;
        };

        Files();
       ~Files();

        // QAbstractTableModel implementation
        virtual QVariant data(const QModelIndex& index, int role) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual void sort(int column, Qt::SortOrder order) override;
        virtual int rowCount(const QModelIndex&) const override;
        virtual int columnCount(const QModelIndex&) const override;

        void loadHistory();
        void eraseHistory();
        void eraseFiles(QModelIndexList& list);

        void keepSorted(bool onOff);

        const File& getItem(std::size_t i) const;

        std::size_t numFiles() const 
        { return files_.size(); }

    public slots:
        void fileCompleted(const app::DataFileInfo& file);

    private:
        std::vector<File> files_;
        bool keepSorted_;
        int sortColumn_;
        int sortOrder_;
    private:
        QFile history_;
    };
    
} // app