// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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

#define LOGTAG "files"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QDir>
#  include <QFile>
#  include <QTextStream>
#include <newsflash/warnpop.h>

#include "eventlog.h"
#include "debug.h"
#include "engine.h"
#include "files.h"
#include "homedir.h"
#include "format.h"

namespace {
    enum class columns {
        type, time, path, name, count
    };
}

namespace app
{

files::files() : keepSorted_(false), sortColumn_(0), sortOrder_(Qt::AscendingOrder)
{
    QObject::connect(g_engine, SIGNAL(fileCompleted(const app::file&)),
        this, SLOT(fileCompleted(const app::file&)));

    DEBUG("files model created");
}

files::~files()
{
    DEBUG("files model deleted");
}

QVariant files::data(const QModelIndex& index, int role) const
{
    // back of the vector is "top of the data"
    // so that we can just push-back instead of more expensive push_front (on deque)
    // and the latest item is then always at table row 0 

    const auto row   = files_.size() - (size_t)index.row()  - 1;
    const auto col   = (columns)index.column();
    const auto& file = files_[row];    

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
            case columns::type: return str(file.type);
            case columns::time: return format(app::event { file.time });
            case columns::path: return file.path;
            case columns::name: return file.name;
            case columns::count: Q_ASSERT(0);
        }
    }
    else if (role == Qt::DecorationRole)
    {
        if (col == columns::type)
            return file.icon;
    }
    return QVariant();
}

QVariant files::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch ((columns)section)
        {
            case columns::type:  return "Type";
            case columns::time:  return "Date";
            case columns::path:  return "Location";
            case columns::name:  return "Name";
            case columns::count: Q_ASSERT(0);
        }
    }
    return QVariant();
}

void files::sort(int column, Qt::SortOrder order) 
{
    if (order == Qt::AscendingOrder)
        DEBUG("Sort in Ascending Order");
    else DEBUG("Sort in Descending Order");

    emit layoutAboutToBeChanged();

    // note that the comparision operators are reversed here
    // because the order of the items in the list is reversed 
    // last item in the list is considered to be the first item
    // on the UI
#define SORT(x) \
    std::sort(std::begin(files_), std::end(files_), \
        [&](const files::file& lhs, const files::file& rhs) { \
            if (order == Qt::AscendingOrder) \
                return lhs.x > rhs.x; \
            return lhs.x < rhs.x; \
        });

    switch ((columns)column)
    {
        case columns::type: SORT(type); break;
        case columns::time: SORT(time); break;
        case columns::path: SORT(path); break;
        case columns::name: SORT(name); break;
        case columns::count: Q_ASSERT(0);        
    }

#undef SORT

    emit layoutChanged();

    sortColumn_ = column;
    sortOrder_  = order;
}

int files::rowCount(const QModelIndex&) const 
{ 
    return (int)files_.size();
}

int files::columnCount(const QModelIndex&) const 
{
    return (int)columns::count;
}

void files::loadHistory()
{
    const auto& file = homedir::file("file-history.txt");

    QFile history;        
    history.setFileName(file);
    if (!history.open(QIODevice::ReadOnly))
    {
        if (!QFileInfo(file).exists())
            return;

        ERROR(str("Unable to read file history _1", history));
        return;
    }

    DEBUG(str("Opened history file _1", history));

    QTextStream load(&history);
    load.setCodec("UTF-8");
    while (!load.atEnd())
    {
        const auto& line = load.readLine();
        if (line.isNull())
            break;

        const auto& toks = line.split("\t");

        files::file next;
        next.type = (filetype)toks[0].toInt();
        next.time = QDateTime::fromTime_t(toks[1].toLongLong());
        next.path = toks[2];
        next.name = toks[3];
        next.icon = find_fileicon(next.type);

        const QFileInfo info(next.path + "/" + next.name);
        if (!info.exists())
            continue;

        files_.push_back(std::move(next));
    }

    history.close();

    // write back out the history data and truncate the file
    // this removes lines of data for files that are not available anymore.
    history_.setFileName(file);
    if (!history_.open(QIODevice::ReadWrite | QIODevice::Truncate))
    {
        ERROR(str("Unable to write file history data _1", history_));
        return;
    }

    QTextStream save(&history_);
    save.setCodec("UTF-8");
    for (const auto& next : files_)
    {
        save << (int)next.type << "\t";
        save << next.time.toTime_t() << "\t";
        save << next.path << "\t";
        save << next.name;
        save << "\n";
    }

    reset();
}

void files::eraseHistory()
{
    if (history_.isOpen())
        history_.close();

    const auto& file = history_.fileName();
    if (file.isEmpty())
        return;

    QFile::remove(file);

    DEBUG(str("Erased file history _1", history_));

    files_.clear();

    reset();
}

void files::eraseFiles(QModelIndexList& list)
{
    qSort(list);

    // remember, index 0 is at the back of the vector
    int removed = 0;

    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row() - removed;

        const auto data = getItem(row);
        const auto file = QString("%1/%2").arg(data.path).arg(data.name);
        QFile::remove(file);
        DEBUG(str("Deleted file _1", file));
        
        QDir dir(data.path);
        QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot);
        if (entries.isEmpty())
        {
            dir.rmdir(data.path);
            DEBUG(str("Deleted directory _1", data.path));
        }

        beginRemoveRows(QModelIndex(), row, row);
        auto it = std::begin(files_);
        std::advance(it, files_.size() - row - 1);
        files_.erase(it);
        endRemoveRows();
        ++removed;
    }
}

void files::keepSorted(bool onOff)
{
    keepSorted_ = onOff;
}

const files::file& files::getItem(std::size_t i) const 
{
    // the vector is accessed in reverse manner (item at index 0 is at the end)
    // so latest item (push_back) comes at the top of the list on the GUI
    return files_[files_.size() - i -1];
}

void files::fileCompleted(const app::file& file)
{
    files::file next;
    next.name = file.name;
    next.path = file.path;
    next.type = find_filetype(file.name);
    next.icon = find_fileicon(next.type);
    next.time = QDateTime::currentDateTime();

    if (!history_.isOpen())
    {
        const auto& file = homedir::file("file-history.txt");
        history_.setFileName(file);
        if (!history_.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            ERROR(str("Unable to write file history data _1", history_));
        }
    }

    // record the file download event in the history file.
    if (history_.isOpen())
    {
        QTextStream stream(&history_);
        stream.setCodec("UTF-8");
        stream << (int)next.type << "\t";
        stream << next.time.toTime_t() << "\t";
        stream << next.path << "\t";
        stream << next.name;
        stream << "\n";
        history_.flush();
    }

    beginInsertRows(QModelIndex(), 0, 0);
    files_.push_back(std::move(next));
    endInsertRows();

    if (keepSorted_)
    {
        sort(sortColumn_, (Qt::SortOrder)sortOrder_);
    }
}

} // app
