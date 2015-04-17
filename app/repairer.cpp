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

#define LOGTAG "repair"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#  include <QDir>
#  include <QFileInfo>
#  include <QRegExp>
#include <newsflash/warnpop.h>
#include <algorithm>
#include <limits>
#include <deque>
#include "repairer.h"
#include "archive.h"
#include "debug.h"
#include "eventlog.h"
#include "paritychecker.h"
#include "utility.h"
#include "platform.h"

namespace app
{

QString toString(ParityChecker::FileState status)
{
    using s = ParityChecker::FileState;
    switch (status)
    {
        case s::Loading:     return "Loading";
        case s::Loaded:      return "Loaded";
        case s::Scanning:    return "Scanning";
        case s::Missing:     return "Missing";
        case s::Found:       return "Found";
        case s::Empty:       return "Empty";
        case s::Damaged:     return "Damaged";
        case s::Complete:    return "Complete";
    }
    Q_ASSERT(0);
    return {};
}

QString toString(ParityChecker::FileType type)
{
    using t = ParityChecker::FileType;
    switch (type)
    {
        case t::Target: return "Target";
        case t::Parity: return "Parity";
    }
    return {};
}


QIcon icon(ParityChecker::FileState state)
{
    using s = ParityChecker::FileState;
    switch (state)
    {
        case s::Loading:  return QIcon("icons:ico_bullet_blue.png");
        case s::Loaded:   return QIcon("icons:ico_bullet_green.png");
        case s::Scanning: return QIcon("icons:ico_bullet_blue.png");
        case s::Missing:  return QIcon("icons:ico_bullet_red.png");
        case s::Found:    return QIcon("icons:ico_bullet_green.png");
        case s::Empty:    return QIcon("icons:ico_bullet_grey.png");
        case s::Damaged:  return QIcon("icons:ico_bullet_yellow.png");
        case s::Complete: return QIcon("icons:ico_bullet_green.png");
    }
    Q_ASSERT(0);
    return {};
}


// list of the current and past recovery operations
class Repairer::RecoveryList : public QAbstractTableModel
{
public:
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal)
            return {};

        if (role != Qt::DisplayRole)
            return {};

        switch ((Columns)section)
        {
            case Columns::Status: return "Status";
            case Columns::Name:   return "Name";
            case Columns::Error:  return "Message";
            case Columns::Path:   return "Location";
            case Columns::LAST:   Q_ASSERT(0); 
        }

        return {};
    }

    virtual QVariant data(const QModelIndex& index, int role) const override
    {
        const auto row = index.row();
        const auto col = index.column();

        const auto& arc = list_[row];
        if (role == Qt::DisplayRole)
        {
            switch ((Columns)col)
            {
                case Columns::Status: return toString(arc.state);
                case Columns::Error:  return arc.message;
                case Columns::Name:   return arc.file;
                case Columns::Path:   return arc.path;
                case Columns::LAST:   Q_ASSERT(0);
            }
        }
        else if (role == Qt::DecorationRole)
        {
            if ((Columns)col == Columns::Status)
                return toIcon(arc.state);
        }
        return {};
    }

    virtual int rowCount(const QModelIndex&) const override
    {
        return (int)list_.size();
    }

    virtual int columnCount(const QModelIndex&) const override
    {
        return (int)Columns::LAST;
    }    

    void addRecovery(const Archive& arc)
    {
        const auto rowPos = (int)list_.size();
        beginInsertRows(QModelIndex(), rowPos, rowPos);
        list_.push_back(arc);
        endInsertRows();
    }

    void refresh(std::size_t index)
    {
        auto first = QAbstractTableModel::index(index, 0);
        auto last  = QAbstractTableModel::index(index, (int)Columns::LAST);
        emit dataChanged(first, last);
    }

    static const std::size_t NoSuchRecovery;// = std::numeric_limits<std::size_t>::max();

    std::size_t findArchive(Archive::Status status) const 
    {
        auto it = std::find_if(std::begin(list_), std::end(list_),
            [=](const Archive& arc) {
                return arc.state == status;
            });
        if (it == std::end(list_))
            return NoSuchRecovery;

        return std::distance(std::begin(list_), it);
    }

    std::size_t numRepairs() const 
    {
        return list_.size();
    }

    Archive& getRecovery(std::size_t index) 
    { 
        BOUNDSCHECK(list_, index);
        return list_[index];
    }

    void moveItems(QModelIndexList& list, int direction, int distance)
    {
        auto minIndex = std::numeric_limits<int>::max();
        auto maxIndex = std::numeric_limits<int>::min();

        for (int i=0; i<list.size(); ++i)
        {
            const auto rowBase = list[i].row();
            for (int x=0; x<distance; ++x)
            {
                const auto row = rowBase + x * direction;

                BOUNDSCHECK(list_, row);
                BOUNDSCHECK(list_, row + direction);

                std::swap(list_[row], list_[row + direction]);
                minIndex = std::min(minIndex, row + direction);
                maxIndex = std::max(maxIndex, row + direction);                
            }
            list[i] = QAbstractTableModel::index(rowBase + direction * distance, 0);
        }
        auto first = QAbstractTableModel::index(minIndex, 0);
        auto last  = QAbstractTableModel::index(maxIndex, (int)Columns::LAST);
        emit dataChanged(first, last);
    }

    void killItems(QModelIndexList& killList)
    {
        std::deque<Archive> survivors;
        std::size_t killIndex = 0;

        sortAscending(killList);

        for (std::size_t i=0; i<list_.size(); ++i)
        {
            if (killIndex < (std::size_t)killList.size())
            {
                if (killList[killIndex].row() == (int)i)
                {
                    ++killIndex;
                    continue;
                }
            }
            survivors.push_back(list_[i]);
        }

        list_ = std::move(survivors);

        QAbstractTableModel::reset();
    }

    void killComplete()
    {
        auto it = std::remove_if(std::begin(list_), std::end(list_),
            [](const Archive& arc) {
                return (arc.state == Archive::Status::Success) ||
                       (arc.state == Archive::Status::Error) ||
                       (arc.state == Archive::Status::Failed);
            });
        list_.erase(it, std::end(list_));
        
        QAbstractTableModel::reset();
    }

private:
    enum class Columns {
        Status, Error, Path, Name, LAST
    };
private:
    std::deque<Archive> list_;
};

const std::size_t Repairer::RecoveryList::NoSuchRecovery = std::numeric_limits<std::size_t>::max();

// data view of the files in the current recovery operation
class Repairer::RecoveryData : public QAbstractTableModel
{
public:
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal)
            return {};

        if (role != Qt::DisplayRole)
            return {};

        switch ((Columns)section)
        {
            case Columns::Status: return "Status";
            case Columns::File:   return "File";
            case Columns::LAST:   Q_ASSERT(0);
        }

        return {};
    }

    virtual QVariant data(const QModelIndex& index, int role) const override
    {
        const auto row = index.row();
        const auto col = index.column();

        const auto& data = files_[row];
        if (role == Qt::DisplayRole)
        {
            switch ((Columns)col)
            {
                case Columns::Status: return toString(data.state);
                case Columns::File:   return data.name;
                case Columns::LAST:   Q_ASSERT(0);
            }
        }
        if (role == Qt::DecorationRole)
        {
            if ((Columns)col == Columns::Status)
                return icon(data.state);
        }
        return {};
    }

    virtual int rowCount(const QModelIndex&) const override
    {
        return (int)files_.size();
    }

    virtual int columnCount(const QModelIndex&) const override
    {
        return (int)Columns::LAST;
    }    

    void update(const app::Archive& arc, ParityChecker::File file)
    {
        //DEBUG("Update file %1, type %2 state %3", file.name, toString(file.type), toString(file.state));

        auto& files = files_;

        auto it = std::find_if(std::begin(files), std::end(files),
            [&](const ParityChecker::File& f) {
                return f.name == file.name;
            });

        if (it == std::end(files))
        {
            const auto curSize = (int)files.size();
            QAbstractTableModel::beginInsertRows(QModelIndex(), curSize, curSize);
            files.push_back(std::move(file));
            QAbstractTableModel::endInsertRows();
        }
        else
        {
            *it = file;

            const auto index = (int)std::distance(std::begin(files), it);
            const auto first = QAbstractTableModel::index(index, 0);
            const auto last  = QAbstractTableModel::index(index, (int)Columns::LAST);
            emit dataChanged(first, last);
        }
    }

    void clear()
    {
        files_.clear();
        reset();
    }

    void setContent(quint32 guid, std::vector<ParityChecker::File> files)
    {
        files_ = std::move(files);
        guid_  = guid;
    }
    quint32 getContentGuid() const 
    { return guid_; }
private:
    enum class Columns {
        Status, /* Done */ File, LAST
    };

private:
    std::vector<ParityChecker::File> files_;
    quint32 guid_;
};

Repairer::Repairer(std::unique_ptr<ParityChecker> engine) : engine_(std::move(engine))
{
    DEBUG("Repairer created");

    list_.reset(new RecoveryList);
    data_.reset(new RecoveryData);

    engine_->onUpdateFile = [=](const Archive& arc, const ParityChecker::File& file) {
        data_->update(arc, file);
        if (info_)
        {
            if (arc.getGuid() == info_->getContentGuid())
                info_->update(arc, file);
        }

        auto ait = std::find_if(std::begin(history_), std::end(history_), 
            [&](const RecoveryFiles& files) {
                return files.archiveID == arc.getGuid();
            });
        ENDCHECK(history_, ait);
        auto& files = (*ait).files;

        auto it = std::find_if(std::begin(files), std::end(files),
            [&](const ParityChecker::File& f) {
                return f.name == file.name;
            });
        if (it == std::end(files))
            files.push_back(file);
        else *it = file;
    };

    engine_->onScanProgress = [=](const Archive& arc, QString file, int done) {
        emit scanProgress(file, done);
    };
    engine_->onRepairProgress = [=](const Archive& arc, QString step, int done) {
        emit repairProgress(step, done);
    };

    engine_->onReady= [=](const Archive& arc) {

        DEBUG("Archive %1 repair ready with status %2", arc.file, toString(arc.state));     
        if (arc.state == Archive::Status::Success)
        {
            NOTE("Archive %1 succesfully repaired.", arc.desc);
            INFO("Archive %1 succesfully repaired.", arc.desc);
        }
        else if (arc.state == Archive::Status::Failed)
        {
            WARN("Archive %1 repair failed.%2", arc.desc, arc.message);
        }
        else if (arc.state == Archive::Status::Error)
        {
            ERROR("Archive %1 repair error. %2", arc.desc, arc.message);
        }

        emit repairReady(arc);

        const auto index = list_->findArchive(Archive::Status::Active);
        if (index != RecoveryList::NoSuchRecovery)
        {
            auto& active = list_->getRecovery(index);
            active = arc;
            list_->refresh(index);
        }

        startNextRecovery();
    };

    writeLogs_ = true;
    purgePars_ = false;
    enabled_   = true;
}

Repairer::~Repairer()
{
    DEBUG("Repairer deleted");
}

QAbstractTableModel* Repairer::getRecoveryData()
{ return data_.get(); }

QAbstractTableModel* Repairer::getRecoveryData(const QModelIndex& index)
{
    if (!info_)
        info_.reset(new RecoveryData);

    const auto row  = index.row();
    const auto& arc = list_->getRecovery(row);
    const auto guid = arc.getGuid();

    const auto it = std::find_if(std::begin(history_), std::end(history_),
        [&](const RecoveryFiles& files) {
            return files.archiveID == guid;
        });
    ENDCHECK(history_, it);

    auto files = (*it).files;
    info_->setContent(guid, std::move(files));
    return info_.get();
}

QAbstractTableModel* Repairer::getRecoveryList()
{ return list_.get(); }


void Repairer::addRecovery(const Archive& arc)
{
    list_->addRecovery(arc);

    RecoveryFiles files;
    files.archiveID = arc.getGuid();
    history_.push_back(files);

    startNextRecovery();
}

void Repairer::stopRecovery()
{
    engine_->stop();
}

void Repairer::moveUp(QModelIndexList& list)
{
    sortAscending(list);

    list_->moveItems(list, -1, 1);
}

void Repairer::moveDown(QModelIndexList& list)
{
    sortDescending(list);

    list_->moveItems(list, 1, 1);
}

void Repairer::moveToTop(QModelIndexList& list)
{
    sortAscending(list);

    const auto dist = list.front().row();

    list_->moveItems(list, -1, dist);
}

void Repairer::moveToBottom(QModelIndexList& list)
{
    sortDescending(list);    

    const auto numRows = (int)list_->numRepairs();
    const auto lastRow = list.first().row();
    const auto dist = numRows - lastRow - 1;

    list_->moveItems(list, 1, dist);
}

void Repairer::kill(QModelIndexList& list)
{
    for (const auto& index : list)
    {
        const auto row  = index.row();
        const auto& arc = list_->getRecovery(row);
        if (arc.state == Archive::Status::Active)
        {
            engine_->stop();
            data_->clear();
        }

        auto it = std::find_if(std::begin(history_), std::end(history_),
            [=](const RecoveryFiles& files) {
                return files.archiveID == arc.getGuid();
            });
        ENDCHECK(history_, it);
        history_.erase(it);
    }
    list_->killItems(list);
}

void Repairer::killComplete()
{
    const auto numRows = list_->numRepairs();
    for (std::size_t i=0; i<numRows; ++i)
    {
        const auto& arc = list_->getRecovery(i);
        if (arc.state == Archive::Status::Error || 
            arc.state == Archive::Status::Success ||
            arc.state == Archive::Status::Failed)
        {
            auto it = std::find_if(std::begin(history_), std::end(history_),
                [=](const RecoveryFiles& files) {
                    return files.archiveID == arc.getGuid();
                });
            ENDCHECK(history_, it);
            history_.erase(it);
        }
    }

    list_->killComplete();

    if (!engine_->isRunning())
        data_->clear();
}

void Repairer::openLog(QModelIndexList& list)
{
    for (int i=0; i<list.size(); ++i)
    {
        const auto row  = list[i].row();
        const auto& arc = list_->getRecovery(row);
        if (arc.state == Archive::Status::Queued)
            continue;
        QFileInfo info(arc.path + "/repair.log");
        if (info.exists())
            app::openFile(info.absoluteFilePath());
    }
}

void Repairer::setWriteLogs(bool onOff)
{ writeLogs_ = onOff; }

void Repairer::setPurgePars(bool onOff)
{ purgePars_ = onOff; }

std::size_t Repairer::numRepairs() const 
{
    return list_->numRepairs();
}

const Archive& Repairer::getRecovery(const QModelIndex& i) const 
{
    return list_->getRecovery(i.row());
}

void Repairer::startNextRecovery()
{
    if (engine_->isRunning())
        return;
    if (!enabled_)
        return;

    const auto index = list_->findArchive(Archive::Status::Queued);
    if (index == RecoveryList::NoSuchRecovery)
    {
        DEBUG("Repairer all done");
        return;
    }

    ParityChecker::Settings settings;
    settings.purgeOnSuccess = purgePars_;
    settings.writeLogFile   = writeLogs_;

    auto& arc = list_->getRecovery(index);
    arc.state = Archive::Status::Active;

    // note the stupid QProcess can invoke the signals synchronously while
    // QProcess::start() is in the *callstack* when the fucking .exe is not found...
    emit repairStart(arc);
    DEBUG("Started recovery for %1", arc.file);
  
    engine_->recover(arc, settings);

    data_->clear();
    list_->refresh(index);
    
}


} // app
