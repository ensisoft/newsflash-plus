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
    // rename file to "filelist.txt"
    // add version number

    QString migrateFile(int& version)
    {
        const auto& newName = app::homedir::file("filelist.txt");
        const auto& oldName = app::homedir::file("file-history.txt");
        if (QFile::exists(oldName))
        {
            version = 1;
            DEBUG("Rename %1 to %2", oldName, newName);
            if (!QFile::rename(oldName, newName))
                return oldName;

            return newName;
        }
        version = CurrentFileVersion;
        return newName;
    }

}

namespace app
{

Files::Files() : m_keepSorted(false), m_sortColumn(0), m_sortOrder(Qt::AscendingOrder), m_version(0)
{
    DEBUG("Files created");

    int version = 0;

    const auto& file = migrateFile(m_version);

    m_file.setFileName(file);
    if (!m_file.open(QIODevice::ReadWrite))
    {
        WARN("Unable to open filelist file %1, %2", file, m_file.error());
        return;
    }
    if (m_file.size() == 0)
    {
        QTextStream out(&m_file);
        out << CurrentFileVersion << "\n";
        return;
    }

    DEBUG("Opened filelist %1", file);    
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

    const auto row   = m_files.size() - (size_t)index.row()  - 1;
    const auto col   = (columns)index.column();
    const auto& file = m_files[row];    

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
    DEBUG("Sorting in %1 order", order);

    emit layoutAboutToBeChanged();

    // note that the comparision operators are reversed here
    // because the order of the items in the list is reversed 
    // last item in the list is considered to be the first item
    // on the UI
#define SORT(x) \
    std::sort(std::begin(m_files), std::end(m_files), \
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

    m_sortColumn = column;
    m_sortOrder  = order;
}

int Files::rowCount(const QModelIndex&) const 
{ 
    return (int)m_files.size();
}

int Files::columnCount(const QModelIndex&) const 
{
    return (int)columns::count;
}



void Files::loadHistory()
{
    if (!m_file.isOpen())
        return;

    QTextStream load(&m_file);
    load.setCodec("UTF-8");

    if (m_version > 1)
    {
        m_version = load.readLine().toInt();
    }

    DEBUG("File version %1", m_version);

    bool needsPruning = false;

    while (!load.atEnd())
    {
        const auto& line = load.readLine();
        if (line.isNull())
            break;

        const auto& toks = line.split("\t");
        if (toks.size() < 4)
        {
            ERROR("Invalid data.");
            continue;
        }

        Files::File next;
        next.type = (FileType)toks[0].toInt();
        next.time = QDateTime::fromTime_t(toks[1].toLongLong());
        next.path = toks[2];
        next.name = toks[3];
        next.icon = findFileIcon(next.type);

        const QFileInfo info(next.path + "/" + next.name);
        if (!info.exists())
        {
            needsPruning = true;
            continue;
        }

        m_files.push_back(std::move(next));
    }

    if (needsPruning)
    {
        // seek back to the start of the file and write back out
        // the data that is still valid.
        m_file.resize(0);
        m_file.seek(0);

        QTextStream save(&m_file);
        save.setCodec("UTF-8");
        save << CurrentFileVersion << "\n";

        for (const auto& next : m_files)
        {
            save << (int)next.type << "\t";
            save << next.time.toTime_t() << "\t";
            save << next.path << "\t";
            save << next.name;
            save << "\n";
        }

        DEBUG("Pruned file list");
    }

    DEBUG("Loaded files list with %1 files", m_files.size());
}

void Files::eraseHistory()
{
    if (m_file.isOpen())
    {
        m_file.resize(0);
        m_file.seek(0);

        QTextStream out(&m_file);
        out.setCodec("UTF-8");
        out << CurrentFileVersion << "\n";
    }

    QAbstractTableModel::beginResetModel();
    m_files.clear();    
    QAbstractTableModel::reset();
    QAbstractTableModel::endResetModel();
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
        auto it = std::begin(m_files);
        std::advance(it, m_files.size() - row - 1);
        m_files.erase(it);
        endRemoveRows();
        ++removed;
    }
}

void Files::keepSorted(bool onOff)
{
    m_keepSorted = onOff;
}

const Files::File& Files::getItem(std::size_t i) const 
{
    // the vector is accessed in reverse manner (item at index 0 is at the end)
    // so latest item (push_back) comes at the top of the list on the GUI
    return m_files[m_files.size() - i -1];
}

void Files::fileCompleted(const app::FileInfo& file)
{
    Files::File next;
    next.name = file.name;
    next.path = file.path;
    next.type = file.type;
    next.icon = findFileIcon(next.type);
    next.time = QDateTime::currentDateTime();

    if (m_file.isOpen())
    {
        // record the file download event in the history file.
        QTextStream stream(&m_file);
        stream.setCodec("UTF-8");
        stream << (int)next.type << "\t";
        stream << next.time.toTime_t() << "\t";
        stream << next.path << "\t";
        stream << next.name;
        stream << "\n";
        m_file.flush();
    }

    beginInsertRows(QModelIndex(), 0, 0);
    m_files.push_back(std::move(next));
    endInsertRows();

    if (m_keepSorted)
    {
        sort(m_sortColumn, (Qt::SortOrder)m_sortOrder);
    }
}

void Files::packCompleted(const app::FilePackInfo& pack)
{}

} // app
