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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtGui/QIcon>
#  include <QDir>
#  include <QFileInfo>
#  include <QRegExp>
#include "newsflash/warnpop.h"
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

        const auto& arc = mList[row];
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
        return (int)mList.size();
    }

    virtual int columnCount(const QModelIndex&) const override
    {
        return (int)Columns::LAST;
    }

    void addRecovery(const Archive& arc)
    {
        const auto rowPos = (int)mList.size();
        beginInsertRows(QModelIndex(), rowPos, rowPos);
        mList.push_back(arc);
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
        auto it = std::find_if(std::begin(mList), std::end(mList),
            [=](const Archive& arc) {
                return arc.state == status;
            });
        if (it == std::end(mList))
            return NoSuchRecovery;

        return std::distance(std::begin(mList), it);
    }

    std::size_t numRepairs() const
    {
        return mList.size();
    }

    Archive& getRecovery(std::size_t index)
    {
        BOUNDSCHECK(mList, index);
        return mList[index];
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

                BOUNDSCHECK(mList, row);
                BOUNDSCHECK(mList, row + direction);

                std::swap(mList[row], mList[row + direction]);
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

        for (std::size_t i=0; i<mList.size(); ++i)
        {
            if (killIndex < (std::size_t)killList.size())
            {
                if (killList[killIndex].row() == (int)i)
                {
                    ++killIndex;
                    continue;
                }
            }
            survivors.push_back(mList[i]);
        }

        mList = std::move(survivors);

        QAbstractTableModel::reset();
    }

    void killComplete()
    {
        auto it = std::remove_if(std::begin(mList), std::end(mList),
            [](const Archive& arc) {
                return (arc.state == Archive::Status::Success) ||
                       (arc.state == Archive::Status::Error) ||
                       (arc.state == Archive::Status::Failed);
            });
        mList.erase(it, std::end(mList));

        QAbstractTableModel::reset();
    }

private:
    enum class Columns {
        Status, Error, Path, Name, LAST
    };
private:
    std::deque<Archive> mList;
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

Repairer::Repairer(std::unique_ptr<ParityChecker> engine) : mEngine(std::move(engine))
{
    DEBUG("Repairer created");

    mList.reset(new RecoveryList);
    mData.reset(new RecoveryData);

    mEngine->onUpdateFile = [=](const Archive& arc, const ParityChecker::File& file) {
        mData->update(arc, file);
        if (mInfo)
        {
            if (arc.getGuid() == mInfo->getContentGuid())
                mInfo->update(arc, file);
        }

        auto ait = std::find_if(std::begin(mHistory), std::end(mHistory),
            [&](const RecoveryFiles& files) {
                return files.archiveID == arc.getGuid();
            });
        // difficult to replicate bug that sometimes causes the archive not to be found
        // in our list of repairs. Dunno what the f*k is the actual problem.
        if (ait == std::end(mHistory))
            return;

        //ENDCHECK(mHistory, ait);
        auto& files = (*ait).files;

        auto it = std::find_if(std::begin(files), std::end(files),
            [&](const ParityChecker::File& f) {
                return f.name == file.name;
            });
        if (it == std::end(files))
            files.push_back(file);
        else *it = file;
    };

    mEngine->onScanProgress = [=](const Archive& arc, QString file, int done) {
        emit scanProgress(file, done);
    };
    mEngine->onRepairProgress = [=](const Archive& arc, QString step, int done) {
        emit repairProgress(step, done);
    };

    mEngine->onReady= [=](const Archive& arc) {

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

        const auto index = mList->findArchive(Archive::Status::Active);
        if (index != RecoveryList::NoSuchRecovery)
        {
            auto& active = mList->getRecovery(index);
            active = arc;
            mList->refresh(index);
        }

        startNextRecovery();
    };
}

Repairer::~Repairer()
{
    DEBUG("Repairer deleted");
}

QAbstractTableModel* Repairer::getRecoveryData()
{ return mData.get(); }

QAbstractTableModel* Repairer::getRecoveryData(const QModelIndex& index)
{
    if (!mInfo)
        mInfo.reset(new RecoveryData);

    const auto row  = index.row();
    const auto& arc = mList->getRecovery(row);
    const auto guid = arc.getGuid();

    const auto it = std::find_if(std::begin(mHistory), std::end(mHistory),
        [&](const RecoveryFiles& files) {
            return files.archiveID == guid;
        });
    ENDCHECK(mHistory, it);

    auto files = (*it).files;
    mInfo->setContent(guid, std::move(files));
    return mInfo.get();
}

QAbstractTableModel* Repairer::getRecoveryList()
{ return mList.get(); }


void Repairer::addRecovery(const Archive& arc)
{
    mList->addRecovery(arc);

    emit repairEnqueue(arc);

    RecoveryFiles files;
    files.archiveID = arc.getGuid();
    mHistory.push_back(files);

    startNextRecovery();
}

void Repairer::stopRecovery()
{
    app::Archive arc;

    if (mEngine->getCurrentArchiveData(&arc))
    {
        mEngine->stop();
        arc.state   = Archive::Status::Stopped;
        arc.message = "Stopped by user.";

        const auto index = mList->findArchive(Archive::Status::Active);
        if (index != RecoveryList::NoSuchRecovery)
        {
            auto& active = mList->getRecovery(index);
            active = arc;
            mList->refresh(index);
        }

        emit repairReady(arc);
    }

    startNextRecovery();
}

void Repairer::shutdown()
{
    mEngine->stop();
}

void Repairer::moveUp(QModelIndexList& list)
{
    sortAscending(list);

    mList->moveItems(list, -1, 1);
}

void Repairer::moveDown(QModelIndexList& list)
{
    sortDescending(list);

    mList->moveItems(list, 1, 1);
}

void Repairer::moveToTop(QModelIndexList& list)
{
    sortAscending(list);

    const auto dist = list.front().row();

    mList->moveItems(list, -1, dist);
}

void Repairer::moveToBottom(QModelIndexList& list)
{
    sortDescending(list);

    const auto numRows = (int)mList->numRepairs();
    const auto lastRow = list.first().row();
    const auto dist = numRows - lastRow - 1;

    mList->moveItems(list, 1, dist);
}

void Repairer::kill(QModelIndexList& list)
{
    for (const auto& index : list)
    {
        const auto row  = index.row();
        const auto& arc = mList->getRecovery(row);
        if (arc.state == Archive::Status::Active)
        {
            mEngine->stop();
            mData->clear();
        }

        auto it = std::find_if(std::begin(mHistory), std::end(mHistory),
            [=](const RecoveryFiles& files) {
                return files.archiveID == arc.getGuid();
            });
        ENDCHECK(mHistory, it);
        mHistory.erase(it);
    }
    mList->killItems(list);
}

void Repairer::killComplete()
{
    const auto numRows = mList->numRepairs();
    for (std::size_t i=0; i<numRows; ++i)
    {
        const auto& arc = mList->getRecovery(i);
        if (arc.state == Archive::Status::Error ||
            arc.state == Archive::Status::Success ||
            arc.state == Archive::Status::Failed)
        {
            auto it = std::find_if(std::begin(mHistory), std::end(mHistory),
                [=](const RecoveryFiles& files) {
                    return files.archiveID == arc.getGuid();
                });
            ENDCHECK(mHistory, it);
            mHistory.erase(it);
        }
    }

    mList->killComplete();

    if (!mEngine->isRunning())
        mData->clear();
}

void Repairer::openLog(QModelIndexList& list)
{
    for (int i=0; i<list.size(); ++i)
    {
        const auto row  = list[i].row();
        const auto& arc = mList->getRecovery(row);
        if (arc.state == Archive::Status::Queued)
            continue;
        QFileInfo info(arc.path + "/repair.log");
        if (info.exists())
            app::openFile(info.absoluteFilePath());
    }
}

std::size_t Repairer::numRepairs() const
{
    return mList->numRepairs();
}

const Archive& Repairer::getRecovery(const QModelIndex& i) const
{
    return mList->getRecovery(i.row());
}

void Repairer::startNextRecovery()
{
    if (mEngine->isRunning())
        return;
    if (!mEnabled)
        return;

    const auto index = mList->findArchive(Archive::Status::Queued);
    if (index == RecoveryList::NoSuchRecovery)
    {
        DEBUG("Repairer all done");
        return;
    }

    ParityChecker::Settings settings;
    settings.purgeOnSuccess = mPurgePars;
    settings.writeLogFile   = mWriteLogs;

    auto& arc = mList->getRecovery(index);
    arc.state = Archive::Status::Active;

    // note the stupid QProcess can invoke the signals synchronously while
    // QProcess::start() is in the *callstack* when the fucking .exe is not found...
    emit repairStart(arc);
    DEBUG("Started recovery for %1", arc.file);

    mEngine->recover(arc, settings);

    mData->clear();
    mList->refresh(index);

}


} // app
