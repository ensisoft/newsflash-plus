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
    class Settings;

    class Groups : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum class Columns {
            messages, subscribed, name, last
        };

        Groups();
       ~Groups();

        // QAbstractTableModel
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual QVariant data(const QModelIndex& index, int role) const override;
        virtual void sort(int column, Qt::SortOrder order) override;
        virtual int rowCount(const QModelIndex&) const override;
        virtual int columnCount(const QModelIndex&) const override;

        void clear();
        void loadListing(const QString& file, quint32 account);
        void makeListing(const QString& file, quint32 account);

        void subscribe(QModelIndexList& list, quint32 account);
        void unsubscribe(QModelIndexList& list, quint32 account);

        void setShowSubscribedOnly(bool on);

    signals:
        void progressUpdated(quint32 acc, quint32 maxValue, quint32 curValue);   
        void loadComplete(quint32 acc);
        void makeComplete(quint32 acc);

    private slots:
        void listingCompleted(quint32 acc, const QList<app::NewsGroup>& list);

    private:
        void setAccountSubscriptions(quint32 accountId);

        enum Flags {
            Subscribed = 0x1
        };

        struct group {
            QString   name;
            quint64   size;
            quint32   flags;
        };
        struct operation {
            quint32 account;
            quint32 batchId;
            QString file;
        };

    private:
        std::vector<group> groups_;
        std::size_t size_;
        std::map<quint32, operation> pending_;
    };

} // app
