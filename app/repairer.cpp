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
#include "repairer.h"
#include "archive.h"
#include "debug.h"
#include "eventlog.h"
#include "datainfo.h"
#include "paritychecker.h"

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

        const auto& arc = list_[row];
        if (role == Qt::DisplayRole)
        {
            switch ((Columns)col)
            {
                case Columns::Status: return toString(arc.state);
                case Columns::Error:  return arc.message;
                case Columns::Desc:   return arc.desc;
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

    static const std::size_t NoSuchRecovery = std::numeric_limits<std::size_t>::max();

    std::size_t findNextPending() const 
    {
        auto it = std::find_if(std::begin(list_), std::end(list_),
            [](const Archive& arc) {
                return arc.state == Archive::Status::Queued;
            });
        if (it == std::end(list_))
            return NoSuchRecovery;

        return std::distance(std::begin(list_), it);
    }

    std::size_t findActive() const 
    {
        auto it = std::find_if(std::begin(list_), std::end(list_),
            [](const Archive& arc) {
                return arc.state == Archive::Status::Active;
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

private:
    enum class Columns {
        Status, Error, Desc, Path, LAST
    };
private:
    std::deque<Archive> list_;
};


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

    void update(const app::Archive&, ParityChecker::File file)
    {
        Q_ASSERT(file.name.isEmpty() == false);

        //DEBUG("Update file %1, type %2 state %3", file.name, toString(file.type), toString(file.state));

        auto it = std::find_if(std::begin(files_), std::end(files_),
            [&](const ParityChecker::File& f) {
                return f.name == file.name;
            });
        if (it == std::end(files_))
        {
            const auto curSize = (int)files_.size();
            beginInsertRows(QModelIndex(), curSize, curSize);
            files_.push_back(std::move(file));
            endInsertRows();
            return;
        }
        *it = file;

        //reset();
        const auto index = (int)std::distance(std::begin(files_), it);
        Q_ASSERT(index < files_.size());
        const auto first = QAbstractTableModel::index(index, 0);
        const auto last  = QAbstractTableModel::index(index, (int)Columns::LAST);
        emit dataChanged(first, last);
    }

    void clear()
    {
        files_.clear();
        reset();
    }

private:
    enum class Columns {
        Status, /* Done */ File, LAST
    };

private:
    std::vector<ParityChecker::File> files_;
};

Repairer::Repairer(std::unique_ptr<ParityChecker> engine) : engine_(std::move(engine))
{
    DEBUG("Repairer created");

    list_.reset(new RecoveryList);
    data_.reset(new RecoveryData);

    engine_->onUpdateFile = std::bind(&RecoveryData::update, data_.get(),
        std::placeholders::_1, std::placeholders::_2);

    engine_->onScanProgress = [=](const Archive& arc, QString file, int done) {
        emit scanProgress(file, done);
    };
    engine_->onRepairProgress = [=](const Archive& arc, QString step, int done) {
        emit repairProgress(step, done);
    };

    engine_->onReady= [=](const Archive& arc) {

        DEBUG("Archive %1 ready with status %2", arc.file, toString(arc.state));        
        emit recoveryReady(arc);

        const auto index = list_->findActive();
        if (index != RecoveryList::NoSuchRecovery)
        {
            auto& active = list_->getRecovery(index);
            active = arc;
            list_->refresh(index);
        }

        startNextRecovery();
    };

}

Repairer::~Repairer()
{
    DEBUG("Repairer deleted");
}

QAbstractTableModel* Repairer::getRecoveryData()
{ return data_.get(); }

QAbstractTableModel* Repairer::getRecoveryList()
{ return list_.get(); }


void Repairer::addRecovery(const Archive& arc)
{
    list_->addRecovery(arc);

    startNextRecovery();
}

void Repairer::stopRecovery()
{
    engine_->stop();
}

std::size_t Repairer::numRepairs() const 
{
    return list_->numRepairs();
}

void Repairer::fileCompleted(const app::DataFileInfo& info)
{}


void Repairer::batchCompleted(const app::FileBatchInfo& info)
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

        Archive arc;
        arc.path  = info.path;
        arc.desc  = file.fileName();
        arc.state = Archive::Status::Queued;
        arc.file  = file.fileName();
        addRecovery(arc);

        DEBUG("Added a new recovery for %1 in %2", name, dir);
    }
    startNextRecovery();
}

void Repairer::startNextRecovery()
{
    if (engine_->isRunning())
        return;

    const auto index = list_->findNextPending();
    if (index == RecoveryList::NoSuchRecovery)
    {
        DEBUG("All Done!");
        emit allDone();
        return;
    }

    // todo:
    ParityChecker::Settings settings;

    auto& recovery = list_->getRecovery(index);
    recovery.state = Archive::Status::Active;
    engine_->recover(recovery, settings);

    data_->clear();    
    list_->refresh(index);

    emit recoveryStart(recovery);
    DEBUG("Started recovery for %1", recovery.desc);    
}


} // app
