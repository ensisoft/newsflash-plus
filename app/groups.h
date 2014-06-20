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

#include <newsflash/sdk/model.h>

#include <newsflash/warnpush.h>
#  include <QAbstractTableModel>
#  include <QDateTime>
#  include <QString>
#  include <QList>
#include <newsflash/warnpop.h>

#include <vector>

namespace app
{
    class groups : public sdk::model, public QAbstractTableModel
    {

    public:
        groups();
       ~groups();

        virtual void save(sdk::datastore& values) const override;

        virtual void load(const sdk::datastore& values) override;

        // sdk::model
        virtual QAbstractItemModel* view() override;

        virtual QString name() const override;

        // QAbstractTableModel
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

        // QAbstractTableMode
        QVariant data(const QModelIndex& index, int role) const override;

        // QAbstractTableModel
        int rowCount(const QModelIndex&) const override;

        // QAbstractTableModel
        int columnCount(const QModelIndex&) const override;

    private:
        enum class column {
            name, updated, headers, articles, size, last
        };

        struct group {
            QString   name;
            QDateTime updated;
            quint64   headers;            
            quint64   articles;
            quint64   size_on_disk;
        };

    private:
        std::vector<group> groups_;
    };

} // app