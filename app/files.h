// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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
#  include <QObject>
#  include <QAbstractTableModel>
#  include <QString>
#  include <QDateTime>
#  include <QFile>
#include <newsflash/warnpop.h>

#include <vector>
#include "engine.h"
#include "filetype.h"

namespace app
{
    class files : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        struct file {
            QString   name;
            QString   path;
            QDateTime time;
            QIcon     icon;
            filetype  type;
        };

        files();
       ~files();

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

        const file& getItem(std::size_t i) const;

    private slots:
        void fileCompleted(const app::DataFile& file);

    private:
        std::vector<file> files_;
        bool keepSorted_;
        int sortColumn_;
        int sortOrder_;
    private:
        QFile history_;
    };
} // app