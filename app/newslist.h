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
#  include <QDateTime>
#  include <QString>
#  include <QList>
//#include <newsflash/warnpop.h>
#include <vector>
#include <map>

namespace app
{
    struct NewsGroupInfo;
    class Settings;

    class NewsList : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum class Columns {
            Messages, SizeOnDisk, Subscribed, Name, LAST
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
        void loadListing(const QString& file, quint32 account);
        void makeListing(const QString& file, quint32 account);

        void subscribe(QModelIndexList& list, quint32 account);
        void unsubscribe(QModelIndexList& list, quint32 account);

        void clearSize(const QModelIndex& index);

        QString getName(const QModelIndex& index) const;
        QString getName(std::size_t index) const;

        std::size_t numItems() const;

        void filter(const QString& str, bool showEmpty);

    signals:
        void progressUpdated(quint32 acc, quint32 maxValue, quint32 curValue);   
        void loadComplete(quint32 acc);
        void makeComplete(quint32 acc);

    private slots:
        void listCompleted(quint32 acc, const QList<app::NewsGroupInfo>& list);
        void newHeaderDataAvailable(const QString& file);

    private:
        void setAccountSubscriptions(quint32 accountId);

        enum Flags {
            Subscribed = 0x1
        };

        struct group {
            QString   name;
            quint64   numMessages;
            quint64   sizeOnDisk;
            quint32   flags;
        };
        struct operation {
            quint32 account;
            quint32 batchId;
            QString file;
        };

    private:
        Columns sort_;
        Qt::SortOrder order_;
        std::size_t size_;
        std::vector<group> groups_;
        std::map<quint32, operation> pending_;
    private:
        quint32 account_;
    };

} // app
