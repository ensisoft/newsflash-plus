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
#include <limits>
#include <deque>
#include "repair.h"
#include "debug.h"
#include "eventlog.h"
#include "engine.h"
#include "distdir.h"
#include "parstate.h"
#include "datainfo.h"

namespace app
{

QString string(Recovery::Status status)
{
    using s = Recovery::Status;
    switch (status)
    {
        case s::Queued:  return "Queued";
        case s::Active:  return "Active";
        case s::Success: return "Success";
        case s::Error:   return "Error";
        case s::Failed:  return "Failed";
    }
    Q_ASSERT(0);
    return {};
}

QIcon icon(Recovery::Status status)
{
    using s = Recovery::Status;
    switch (status)
    {
        case s::Queued:  return QIcon("icons:ico_recovery_queued.png");
        case s::Active:  return QIcon("icons:ico_recovery_active.png");
        case s::Success: return QIcon("icons:ico_recovery_success.png");
        case s::Error:   return QIcon("icons:ico_recovery_error.png");
        case s::Failed:  return QIcon("icons:ico_recovery_failed.png");
    }    
    Q_ASSERT(0);
    return {};
}

QString string(ParState::FileState status)
{
    using s = ParState::FileState;
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


QIcon icon(ParState::FileState state)
{
    using s = ParState::FileState;
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
class RepairEngine::RecoveryList : public QAbstractTableModel
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
            case Columns::Desc:   return "Desc";
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

        const auto& rec = list_[row];
        if (role == Qt::DisplayRole)
        {
            switch ((Columns)col)
            {
                case Columns::Status: return string(rec.state);
                case Columns::Error:  return rec.message;
                case Columns::Desc:   return rec.desc;
                case Columns::Path:   return rec.path;
                case Columns::LAST:   Q_ASSERT(0);
            }
        }

        if (role == Qt::DecorationRole)
        {
            if ((Columns)col == Columns::Status)
                return icon(rec.state);
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

    void addRecovery(const Recovery& rec)
    {
        const auto rowPos = (int)list_.size();
        beginInsertRows(QModelIndex(), rowPos, rowPos);
        list_.push_back(rec);
        endInsertRows();
    }

    void refresh(std::size_t index)
    {
        auto first = QAbstractTableModel::index(index, 0);
        auto last  = QAbstractTableModel::index(index, (int)Columns::LAST);
        emit dataChanged(first, last);
    }

    static const std::size_t NoSuchRecovery = std::numeric_limits<std::size_t>::max();

    std::size_t findNextPending() const 
    {
        auto it = std::find_if(std::begin(list_), std::end(list_),
            [](const Recovery& rec) {
                return rec.state == Recovery::Status::Queued;
            });
        if (it == std::end(list_))
            return NoSuchRecovery;

        return std::distance(std::begin(list_), it);
    }

    std::size_t findActive() const 
    {
        auto it = std::find_if(std::begin(list_), std::end(list_),
            [](const Recovery& rec) {
                return rec.state == Recovery::Status::Active;
            });
        if (it == std::end(list_))
            return NoSuchRecovery;

        return std::distance(std::begin(list_), it);
    }

    Recovery& getRecovery(std::size_t index) 
    { 
        Q_ASSERT(index < list_.size());
        return list_[index];
    }

private:
    enum class Columns {
        Status, Error, Desc, Path, LAST
    };
private:
    friend class RepairEngine;
    std::deque<Recovery> list_;
};


// data view of the files in the current recovery operation
class RepairEngine::RecoveryData : public QAbstractTableModel
{
public:
    RecoveryData() : state_(files_)
    {
        state_.onInsert = [=] { 
            this->reset(); 
        };
    }

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
                case Columns::Status: return string(data.state);
                case Columns::File:   return data.file;
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

    float update(const QString& line)
    {
        state_.update(line);

        //const auto numFiles = (int)files_.size();
        //auto first = QAbstractTableModel::index(numFiles, 0);
        //auto last  = QAbstractTableModel::index(numFiles, (int)Columns::LAST);
        //emit dataChanged(first, last);
        //return newProgress;
        return 0;
    }

    void clear()
    {
        files_.clear();
        state_.clear();
        reset();
    }

    ParState::ExecState execState() const 
    { return state_.getState(); }

private:
    enum class Columns {
        Status, /* Done */ File, LAST
    };

private:
    friend class RepairEngine;
    std::vector<ParState::File> files_;
    ParState state_;
};

RepairEngine::RepairEngine()
{
    DEBUG("RepairEngine created");

    QObject::connect(&process_, SIGNAL(finished(int, QProcess::ExitStatus)), 
        this, SLOT(processFinished(int, QProcess::ExitStatus)));
    QObject::connect(&process_, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(processError(QProcess::ProcessError)));
    QObject::connect(&process_, SIGNAL(readyReadStandardOutput()),
        this, SLOT(processStdOut()));
    QObject::connect(&process_, SIGNAL(readyReadStandardError()),
        this, SLOT(processStdErr()));        

    QObject::connect(g_engine, SIGNAL(fileCompleted(const app::DataFileInfo&)),
        this, SLOT(fileCompleted(const app::DataFileInfo&)));
    QObject::connect(g_engine, SIGNAL(batchCompleted(const app::FileBatchInfo&)),
        this, SLOT(batchCompleted(const app::FileBatchInfo&)));    

    list_.reset(new RecoveryList);
    data_.reset(new RecoveryData);
}

RepairEngine::~RepairEngine()
{
    DEBUG("RepairEngine deleted");
}

QAbstractTableModel* RepairEngine::getRecoveryData()
{ return data_.get(); }

QAbstractTableModel* RepairEngine::getRecoveryList()
{ return list_.get(); }


void RepairEngine::addRecovery(const Recovery& rec)
{
    list_->addRecovery(rec);

    startNextRecovery();
}

void RepairEngine::stopRecovery()
{
    const auto state = process_.state();
    if (state == QProcess::NotRunning)
        return;

    process_.terminate();
}

void RepairEngine::processStdOut()
{
    const auto buff = process_.readAllStandardOutput();
    const auto size = buff.size();
    stdout_.append(buff);

    std::string line;

    for (int i=0; i<stdout_.size(); ++i)
    {
        const auto byte = stdout_.at(i);
        if (byte == '\r' || byte == '\n')
        {
            const auto wide = widen(line);
            const auto exec = data_->execState();            
            data_->update(wide);
            if (exec == ParState::ExecState::Scan)
            {
                QString file;
                float   done;
                ParState::parseFileProgress(wide, file, done);
                emit scanProgress(file, done);
            }
            else if (exec == ParState::ExecState::Repair)
            {
                QString step;
                float done;
                ParState::parseRepairProgress(wide, step, done);
                emit repairProgress(step, done);
            }
            line.clear();
        } else line.push_back(byte);
    }
    const auto leftOvers = (int)line.size();

    stdout_ = stdout_.right(leftOvers);
}

void RepairEngine::processStdErr()
{
    Q_ASSERT(!"We should be using merged IO channels for stdout and stderr."
        "See the code in startNextRecovery()");
}

void RepairEngine::processFinished(int exitCode, QProcess::ExitStatus status)
{
    const auto index = list_->findActive();
    if (index == RecoveryList::NoSuchRecovery)
        return;

    // deal with any remaining input if any
    processStdOut();

    auto& active = list_->getRecovery(index);

    DEBUG("Recovery '%1' finished. ExitCode %2, ExitStatus %3",
        active.desc, exitCode, status);

    if (status == QProcess::CrashExit)
    {
        active.state   = Recovery::Status::Error;
        active.message = "Crash exit";
        ERROR("Repair '%1' error %2", active.desc, active.message);
    }
    else
    {
        const auto state   = data_->state_.getState();
        const auto message = data_->state_.getMessage();
        const auto success = data_->state_.getSuccess();
        Q_ASSERT(state == ParState::ExecState::Finish);

        active.message = message;

        if (success)
        {
            active.state = Recovery::Status::Success;
            INFO("Repair '%1' complete!", active.desc);
            NOTE("Repair '%1' complete!", active.desc);
        }
        else 
        {
            active.state = Recovery::Status::Failed;
            WARN("Repair '%1' failed. %2", active.desc, active.message);
        }
    }
    list_->refresh(index);

    emit recoveryReady(active);

    startNextRecovery();

}

void RepairEngine::processError(QProcess::ProcessError error)
{
    DEBUG("ProcessError %1", error);

    const auto index = list_->findActive();
    if (index == RecoveryList::NoSuchRecovery)
        return;

    auto& active = list_->getRecovery(index);

    active.state   = Recovery::Status::Error;
    active.message = toString(error);
    ERROR("Repair '%1' error %2", active.desc, active.message);

    list_->refresh(index);

    recoveryReady(active);

    startNextRecovery();
}


void RepairEngine::fileCompleted(const app::DataFileInfo& info)
{}


void RepairEngine::batchCompleted(const app::FileBatchInfo& info)
{
    DEBUG("Repair a batch %1", info.path);

    QDir dir;
    dir.setPath(info.path);
    dir.setNameFilters(QStringList("*.par2"));

    QStringList files = dir.entryList();
    for (int i=0; i<files.size(); ++i)
    {
        QFileInfo file(files[i]);
        QString   name = file.completeBaseName();
        if (name.contains(".vol"))
            continue;

        Recovery rec;
        rec.path  = info.path;
        rec.desc  = file.fileName();
        rec.state = Recovery::Status::Queued;
        rec.file  = file.fileName();
        addRecovery(rec);

        DEBUG("Added a new recovery for %1 in %2", name, dir);
    }
    startNextRecovery();
}

void RepairEngine::startNextRecovery()
{
    const auto state = process_.state();
    if (state != QProcess::NotRunning)
        return;

    const auto index = list_->findNextPending();
    if (index == RecoveryList::NoSuchRecovery)
    {
        DEBUG("All Done!");
        emit allDone();
        return;
    }

    data_->clear();    
    stdout_.clear();
    stderr_.clear();

    auto& recovery = list_->getRecovery(index);

    const auto& par2 = distdir::path("/par2");
    const auto& file = recovery.path + "/" + recovery.file;

    QStringList args;
    args << "r"; // for repair
    args << recovery.file;
    args << "./*";
    process_.setWorkingDirectory(recovery.path);
    process_.setProcessChannelMode(QProcess::MergedChannels);
    process_.start(par2, args);
    DEBUG("Starting recovery for %1", recovery.desc);

    recovery.state = Recovery::Status::Active;
    list_->refresh(index);

    emit recoveryStart(recovery);
}

RepairEngine* g_repair;


} // app
