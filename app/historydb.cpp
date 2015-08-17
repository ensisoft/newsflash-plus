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
#  include <QTextStream>
#  include <QFileInfo>
#include <newsflash/warnpop.h>
#include <algorithm>
#include "historydb.h"
#include "debug.h"
#include "eventlog.h"
#include "format.h"
#include "homedir.h"
#include "types.h"
#include "download.h"
#include "settings.h"

namespace app
{

HistoryDb::HistoryDb() : m_loaded(false), m_checkDuplicates(true), m_exactMatch(false)
{
    DEBUG("HistoryDb created");
}

HistoryDb::~HistoryDb()
{
    DEBUG("HistoryDb deleted");
}

QVariant HistoryDb::data(const QModelIndex& index, int role) const
{
    // back of the vector is the "top of the data"
    const auto row = m_items.size() - (size_t)index.row() -1 ;
    const auto col = (Columns)index.column();

    const auto& item = m_items[row];

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
            case Columns::Date: 
                return toString(app::event{item.date});

            case Columns::Type: 
                return toString(item.type);

            case Columns::Desc: 
                return item.desc; 

            case Columns::SENTINEL: 
                Q_ASSERT(0);
        }
    }
    else if (role == Qt::DecorationRole)
    {
        if (col == Columns::Date)
        {
            return toIcon(item.source);
        }
    }
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
    // todo: is this data even needed to be sortable?
}

int HistoryDb::rowCount(const QModelIndex&) const 
{
    return (int)m_items.size();
}


int HistoryDb::columnCount(const QModelIndex&) const 
{
    return (int)Columns::SENTINEL;
}

void HistoryDb::loadState(Settings& settings)
{
    m_checkDuplicates = settings.get("history", "check_duplicates", m_checkDuplicates);
    m_exactMatch = settings.get("history", "exact_matching", m_exactMatch);
}

void HistoryDb::saveState(Settings& settings) const
{
    settings.set("history", "check_duplicates", m_checkDuplicates);
    settings.set("history", "exact_matching", m_exactMatch);
}

void HistoryDb::loadHistory()
{
    const auto& file = homedir::file("history.txt");

    m_file.setFileName(file);
    m_file.open(QIODevice::ReadWrite | QIODevice::Append);
    if (!m_file.isOpen())
    {
        if (!QFile::exists(file))
            return;

        ERROR("Unable to load history data %1, %2", file, m_file.error());
        return;
    }

    DEBUG("Opened history file %1", file);

    m_items.clear();
    m_file.seek(0);

    QTextStream stream(&m_file);
    stream.setCodec("UTF-8");
    while (!stream.atEnd())
    {
        const auto& line = stream.readLine();
        if (line.isNull())
            break;

        QStringList toks = line.split("\t");

        HistoryDb::Item item;
        item.source = (MediaSource)toks[0].toInt();
        item.type   = (MediaType)toks[1].toInt();
        item.date   = QDateTime::fromTime_t(toks[2].toLongLong());
        item.desc   = toks[3];
        m_items.push_back(item);
    }

    DEBUG("Loaded history data with %1 items", m_items.size());

    m_loaded = true;
}

void HistoryDb::unloadHistory()
{
    m_items.clear();
    m_loaded = false;
}

void HistoryDb::clearHistory(bool commit)
{
    if (commit)
    {
        if (m_file.isOpen())
            m_file.close();

        const auto& file = homedir::file("history.txt");

        m_file.open(QIODevice::Truncate | QIODevice::WriteOnly);
    }

    if (m_loaded)
    {
        QAbstractTableModel::beginResetModel();
        m_items.clear();
        QAbstractTableModel::reset();
        QAbstractTableModel::endResetModel();
    }
}

bool HistoryDb::isEmpty() const
{
    return m_items.empty();
}

bool HistoryDb::isLoaded() const 
{
    return m_loaded;
}

bool HistoryDb::checkDuplicates() const
{
    return m_checkDuplicates;
}

bool HistoryDb::exactMatching() const
{
    return m_exactMatch;
}

void HistoryDb::checkDuplicates(bool onOff) 
{
    m_checkDuplicates = onOff;
}

void HistoryDb::exactMatching(bool onOff)
{
    m_exactMatch = onOff;
}

bool HistoryDb::lookup(const QString& desc, MediaType type, Item* item) const 
{
    auto it = std::find_if(std::begin(m_items), std::end(m_items), 
        [&](const Item& i) {
            return i.desc == desc && i.type == type;
        });
    if (it == std::end(m_items))
        return false;

    if (item)
        *item = *it;

    return true;
}

bool HistoryDb::isDuplicate(const QString& desc, MediaType type, Item* item) const 
{
    if (!m_checkDuplicates)
        return false;

    if (!m_loaded)
        const_cast<HistoryDb*>(this)->loadHistory();

    if (m_exactMatch)
        return matchExactly(desc, type, item);

    if (isMovie(type))
    {
        QString title = findMovieTitle(desc);
        if (!title.isEmpty())
        {
            DEBUG("Checking Movie title %1", title);

            title = title.toLower();
            for (const auto& item : m_items)
            {
                if (!isMovie(item.type))
                    continue;
                QString t = findMovieTitle(item.desc);
                if (t.toLower() == title)
                    return true;
            }
        }
    }
    else if (isTelevision(type))
    {
        QString season;
        QString episode;
        QString title = findTVSeriesTitle(desc, &season, &episode);
        if (!title.isEmpty())
        {
            DEBUG("Checking Television title %1", title);

            title = title.toLower();
            for (const auto& item : m_items)
            {
                if (!isTelevision(item.type))
                    continue;
                QString e, s;
                QString t = findTVSeriesTitle(item.desc, &s, &e);
                if (s != season || e != episode)
                    continue;
                if (t.toLower() == title)
                    return true;
            }
        }
    }
    else if (isAdult(type))
    {
        QString title = findAdultTitle(desc);
        if (!title.isEmpty())
        {
            DEBUG("Checking Adult title %1", title);

            title = title.toLower();
            for (const auto& item : m_items)
            {
                if (!isAdult(item.type))
                    continue;

                QString t = findAdultTitle(item.desc);
                if (t.toLower() == title)
                    return true;
            }
        }
    }

    return matchExactly(desc, type, item);


}

void HistoryDb::newDownloadQueued(const Download& download)
{
    if (!m_file.isOpen())
    {
        const auto& file = homedir::file("history.txt");
        m_file.setFileName(file);
        m_file.open(QIODevice::WriteOnly | QIODevice::Append);
        if (!m_file.isOpen())
        {
            ERROR("Unable to open history file %1, %2", file, m_file.error());
            return;
        }
    }
    HistoryDb::Item item;
    item.date   = QDateTime::currentDateTime();
    item.desc   = download.desc;
    item.type   = download.type;
    item.source = download.source;

    QTextStream stream(&m_file);
    stream.setCodec("UTF-8");
    stream << (int)item.source << "\t";
    stream << (int)item.type   << "\t";
    stream << item.date.toTime_t() << "\t";
    stream << item.desc;
    stream << "\n";

    m_file.flush();

    if (m_loaded)
    {
        beginInsertRows(QModelIndex(), 0, 0);
        m_items.push_back(item);
        endInsertRows();
    }
}

bool HistoryDb::matchExactly(const QString& desc, MediaType type, Item* item) const 
{
    for (const auto& item : m_items)
    {
        if (item.type != type)
            continue;
        if (item.desc == desc)
            return true;
    }
    return false;

}

HistoryDb* g_history;

} // app