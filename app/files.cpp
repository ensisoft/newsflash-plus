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

#define LOGTAG "files"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QDir>
#  include <QFile>
#  include <QTextStream>
#include <newsflash/warnpop.h>
#include "eventlog.h"
#include "debug.h"
#include "files.h"
#include "homedir.h"
#include "format.h"
#include "types.h"
#include "fileinfo.h"
#include "utility.h"

namespace {
    enum class columns {
        type, time, path, name, count
    };

    const int CurrentFileVersion = 2;

    // Version 1:
    // no version number in the file.
    // filename called "file-history.txt"

    // version 2:
    // rename file to "files.txt"
    // add version number

}

namespace app
{

Files::Files() : keepSorted_(false), sortColumn_(0), sortOrder_(Qt::AscendingOrder)
{
    DEBUG("Files created");
}

Files::~Files()
{
    DEBUG("Files deleted");
}

QVariant Files::data(const QModelIndex& index, int role) const
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
            case columns::type: return toString(file.type);
            case columns::time: return toString(app::event { file.time });
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

QVariant Files::headerData(int section, Qt::Orientation orientation, int role) const
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

void Files::sort(int column, Qt::SortOrder order) 
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
        [&](const Files::File& lhs, const Files::File& rhs) { \
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

int Files::rowCount(const QModelIndex&) const 
{ 
    return (int)files_.size();
}

int Files::columnCount(const QModelIndex&) const 
{
    return (int)columns::count;
}

void Files::loadHistory()
{
    const auto& file = homedir::file("file-history.txt");

    QFile history;        
    history.setFileName(file);
    if (!history.open(QIODevice::ReadOnly))
    {
        if (!QFileInfo(file).exists())
            return;

        WARN("Unable to read file history %1, %2", file, 
            history.error());
        return;
    }

    DEBUG("Opened history file %1", file);

    QTextStream load(&history);
    load.setCodec("UTF-8");
    while (!load.atEnd())
    {
        const auto& line = load.readLine();
        if (line.isNull())
            break;

        const auto& toks = line.split("\t");

        Files::File next;
        next.type = (FileType)toks[0].toInt();
        next.time = QDateTime::fromTime_t(toks[1].toLongLong());
        next.path = toks[2];
        next.name = toks[3];
        next.icon = findFileIcon(next.type);

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
        WARN("Unable to write file history data %1, %2", file, 
            history_.error());
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

void Files::eraseHistory()
{
    if (history_.isOpen())
        history_.close();

    const auto& file = history_.fileName();
    if (file.isEmpty())
        return;

    QFile::remove(file);

    files_.clear();

    reset();
}

void Files::eraseFiles(QModelIndexList& list)
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
        DEBUG("Deleted file %1", file);
        
        QDir dir(data.path);
        QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot);
        if (entries.isEmpty())
        {
            dir.rmdir(data.path);
            DEBUG("Deleted directory %1", data.path);
        }

        beginRemoveRows(QModelIndex(), row, row);
        auto it = std::begin(files_);
        std::advance(it, files_.size() - row - 1);
        files_.erase(it);
        endRemoveRows();
        ++removed;
    }
}

void Files::keepSorted(bool onOff)
{
    keepSorted_ = onOff;
}

const Files::File& Files::getItem(std::size_t i) const 
{
    // the vector is accessed in reverse manner (item at index 0 is at the end)
    // so latest item (push_back) comes at the top of the list on the GUI
    return files_[files_.size() - i -1];
}

void Files::fileCompleted(const app::FileInfo& file)
{
    Files::File next;
    next.name = file.name;
    next.path = file.path;
    next.type = file.type;
    next.icon = findFileIcon(next.type);
    next.time = QDateTime::currentDateTime();

    if (!history_.isOpen())
    {
        const auto& file = homedir::file("file-history.txt");
        history_.setFileName(file);
        if (!history_.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            ERROR("Unable to write file history data %1, %2", file,
                history_.error());
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

void Files::packCompleted(const app::FilePackInfo& pack)
{}

} // app
