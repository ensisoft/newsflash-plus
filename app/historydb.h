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
#include <newsflash/warnpop.h>
#include <vector>
#include "media.h"

namespace app
{
    class HistoryDb : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum class MediaSource {
            RSS, Search, Headers
        };

        struct Item {
            QDateTime date;
            QString desc;
            MediaType type;
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

       void loadHistory();
    private:
        enum class Columns {
            Date, Type, Desc, SENTINEL
        };

    private:
        std::vector<Item> m_items;
    };

    extern HistoryDb* g_history;

} // app