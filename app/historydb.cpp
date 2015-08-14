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

#define LOGTAG "history"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>

#include <newsflash/warnpop.h>
#include "historydb.h"
#include "debug.h"
#include "eventlog.h"
#include "format.h"

namespace app
{

HistoryDb::HistoryDb()
{
    DEBUG("HistoryDb created");
}

HistoryDb::~HistoryDb()
{
    DEBUG("HistoryDb deleted");
}

QVariant HistoryDb::data(const QModelIndex& index, int role) const
{
    return {};
}

QVariant HistoryDb::headerData(int section, Qt::Orientation orientation, int role) const 
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch ((Columns)section)
        {
            case Columns::Date: return "Date";
            case Columns::Type: return "Category";
            case Columns::Desc: return "Title";
            case Columns::SENTINEL: Q_ASSERT(0);
        }
    }    
    return {};
}

void HistoryDb::sort(int column, Qt::SortOrder order)
{

}

int HistoryDb::rowCount(const QModelIndex&) const 
{
    return 0;
}


int HistoryDb::columnCount(const QModelIndex&) const 
{
    return (int)Columns::SENTINEL;
}

void HistoryDb::loadHistory()
{}


HistoryDb* g_history;

} // app