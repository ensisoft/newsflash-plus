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
#  include <QObject>
#  include <QString>
#  include <QDateTime>
#  include <QAbstractTableModel>
#  include <QFile>
#include <newsflash/warnpop.h>
#include <vector>
#include "media.h"

namespace app
{
    struct Download;

    // Record downloads to a historical database.
    class HistoryDb : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        struct Item {
            QDateTime   date;
            QString     desc;
            MediaType   type;
            MediaSource source;
        };

        HistoryDb();
       ~HistoryDb();

       // QAbstractTableModel implementation
       virtual QVariant data(const QModelIndex& index, int role) const override;
       virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
       virtual void sort(int column, Qt::SortOrder order) override;
       virtual int rowCount(const QModelIndex&) const override;
       virtual int columnCount(const QModelIndex&) const override;

       // Load the history data.
       void loadHistory();

       // Unload the history data reduce memory footprint.
       void unloadHistory();

       // clear/remove the current history data. 
       // if commit is true the history database is also
       // eradicated. otherwise only in memory data is only cleared.
       void clearHistory(bool commit);


       // returns true if database is empty.
       bool isEmpty() const;

       // try to lookup an item in the database with
       // direct match on desc and type attributes. 
       // returns true if matching item is found and stores
       // it in item if non-null ptr.
       bool lookup(const QString& desc, MediaType type, Item* item) const;

       bool isDuplicate(const QString& desc, MediaType type, Item* item) const;

    public slots:
        void newDownloadQueued(const Download& download);

    private:
        enum class Columns {
            Date, Type, Desc, SENTINEL
        };

    private:
        std::vector<Item> m_items;
        bool m_loaded;
    private:
        QFile m_file;
    };

    extern HistoryDb* g_history;

} // app