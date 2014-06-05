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

namespace app
{
    class valuestore;

    class groups : public QAbstractTableModel
    {

    public:
        groups();
       ~groups();

        void persist(app::valuestore& values) const;

        void retrieve(const app::valuestore& values);

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;

        int rowCount(const QModelIndex&) const override;
        int columnCount(const QModelIndex&) const override;

    private:
        enum class column {
            name, created, updated, articles, size, last
        };

        struct group {
            QString   name;
            QDateTime created;
            QDateTime updated;
            quint64   size;
        };

    private:
        QList<group> groups_;
    };

} // app
