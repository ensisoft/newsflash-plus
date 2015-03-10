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
#include <newsflash/warnpop.h>
#include <algorithm>
#include <vector>
#include "debug.h"

namespace app
{
    // a template shim to simplify our Qt data model interface.
    template<typename Table, typename Item>
    class TableModel : public QAbstractTableModel, public Table
    {
    public:
        using Columns = typename Table::Columns;

        virtual QVariant data(const QModelIndex& index, int role) const override
        {
            const auto row   = (std::size_t)index.row();
            const auto col   = (Columns)index.column();
            const auto& item = items_[row];
            if (role == Qt::DisplayRole)
                return Table::cellData(item, col);
            if (role == Qt::DecorationRole)
                return Table::cellIcon(item, col);

            return {};
        }

        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override
        {
            if (orientation != Qt::Horizontal)
                return {};

            const auto col = (Columns)section;

            if (role == Qt::DisplayRole)
                return Table::name(col);
            if (role == Qt::DecorationRole)
                return Table::icon(col);
            if (role == Qt::UserRole)
                return Table::size(col);

            return {};
        }

        virtual void sort(int column, Qt::SortOrder order)
        {
            const auto col = (Columns)column;

            emit layoutAboutToBeChanged();
            Table::sort(items_, col, order);
            emit layoutChanged();
        }

        int rowCount(const QModelIndex&) const 
        {
            return static_cast<int>(items_.size());
        }

        int columnCount(const QModelIndex&) const 
        {
            return static_cast<int>(Columns::LAST);
        }
        void clear()
        {
            items_.clear();
            QAbstractTableModel::reset();
        }

        const Item& getItem(std::size_t index) const 
        { 
            BOUNDSCHECK(items_, index);
            return items_[index];
        }
        const Item& getItem(const QModelIndex& index) const 
        {
            BOUNDSCHECK(items_, index.row());
            return items_[index.row()];
        }

        void setItems(std::vector<Item> items)
        {
            items_ = std::move(items);
            QAbstractTableModel::reset();
        }

        std::size_t numItems() const
        {
            return items_.size();
        }

        void append(std::vector<Item> items)
        {
            const auto numItems = rowCount(QModelIndex());
            const auto newItems = items.size();
            beginInsertRows(QModelIndex(), numItems, numItems + newItems);
            std::copy(std::begin(items), std::end(items),
                std::back_inserter(items_));
            endInsertRows();
        }

        bool isEmpty() const
        { 
            return items_.empty(); 
        }
    private:
        std::vector<Item> items_;
    };
} // app

