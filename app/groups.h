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
#  include <QAbstractTableModel>
#  include <QDateTime>
#  include <QString>
#  include <QList>
#include <newsflash/warnpop.h>

#include <vector>
#include <map>

namespace app
{
    struct NewsGroup;

    class Groups : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        Groups();
       ~Groups();

        // QAbstractTableModel
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex&) const override;
        int columnCount(const QModelIndex&) const override;

        void clear();

        void loadListing(const QString& file, quint32 account);
        void makeListing(const QString& file, quint32 account);

    private slots:
        void listingCompleted(quint32 acc, const QList<app::NewsGroup>& list);

    private:
        enum class column {
            name, updated, headers, articles, size, last
        };

        struct group {
            QString   name;
            QDateTime updated;
            quint64   numMessages;
            quint64   sizeOnDisk;
            quint32   flags;
        };

        struct operation {
            quint32 account;
            quint32 taskId;
            QString file;
        };

    private:
        std::vector<group> groups_;
        std::map<quint32, operation> pending_;
        quint32 account_;
    };

} // app
