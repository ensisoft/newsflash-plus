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

#define LOGTAG "unpack"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#  include <QDir>
#  include <QFileInfo>
#  include <QRegExp>
#include <newsflash/warnpop.h>
#include <limits>
#include "unpacker.h"
#include "debug.h"
#include "eventlog.h"
#include "engine.h"
#include "distdir.h"
#include "parstate.h"
#include "archive.h"

namespace app
{

QString string(Archive::Status status)
{
    using s = Archive::Status;
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

class Unpacker::UnpackList : public QAbstractTableModel
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
            // if ((Columns)col == Columns::Status)
            //     return icon(rec.state);
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

    void addUnpack(const Archive& arc)
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

    static const std::size_t NoSuchUnpack = std::numeric_limits<std::size_t>::max();

    std::size_t findNextPending() const 
    {
        auto it = std::find_if(std::begin(list_), std::end(list_),
            [](const Archive& arc) {
                return arc.state == Archive::Status::Queued;
            });
        if (it == std::end(list_))
            return NoSuchUnpack;

        return std::distance(std::begin(list_), it);
    }

    std::size_t findActive() const 
    {
        auto it = std::find_if(std::begin(list_), std::end(list_),
            [](const Archive& arc) {
                return arc.state == Archive::Status::Active;
            });
        if (it == std::end(list_))
            return NoSuchUnpack;

        return std::distance(std::begin(list_), it);
    }

    Archive& getArchive(std::size_t index) 
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
    std::deque<Archive> list_;
};

class Unpacker::UnpackData : public QAbstractTableModel
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
                case Columns::File:   return data.name;
                case Columns::LAST:   Q_ASSERT(0);
            }
        }
        if (role == Qt::DecorationRole)
        {
            //if ((Columns)col == Columns::Status)
            //    return icon(data.state);
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

    bool update(const QString& line)
    {
        static const QRegExp regex("Extracting from (.*)");
        if (regex.indexIn(line) != -1)
        {
            File file;
            file.name = regex.cap(1);

            const auto numRows = (int)files_.size();
            beginInsertRows(QModelIndex(), numRows, numRows);
            files_.push_back(file);
            endInsertRows(); 
            return true;
        }
        return false;
    }

    void clear()
    {
        files_.clear();
        reset();
    }

private:
    enum class Columns {
        File, LAST
    };

    struct File {
        QString name;
    };

private:
    friend class Unpacker;
    std::vector<File> files_;
};


Unpacker::Unpacker()
{
    DEBUG("Unpacker created");

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

    list_.reset(new UnpackList);    
    data_.reset(new UnpackData);
}

Unpacker::~Unpacker()
{
    DEBUG("Unpacker deleted");
}

QAbstractTableModel* Unpacker::getUnpackList()
{ return list_.get(); }

QAbstractTableModel* Unpacker::getUnpackData()
{ return data_.get(); }

void Unpacker::addUnpack(const Archive& arc)
{
    list_->addUnpack(arc);

    startNextUnpack();
}

void Unpacker::stopUnpack()
{
    const auto state = process_.state();
    if (state == QProcess::NotRunning)
        return;

    process_.terminate();
}

void Unpacker::processStdOut()
{
    const auto buff = process_.readAllStandardOutput();
    const auto size = buff.size();
    stdout_.append(buff);

    std::string line;

    int chop;
    for (chop = 0; chop<stdout_.size(); ++chop)
    {
        const auto byte = stdout_.at(chop);
        if (byte == '\n' || byte =='\r')
        {
            const auto wide = widen(line);
            if (wide.isEmpty())
                continue;

            data_->update(widen(line));

//            DEBUG(wide);

            static const QRegExp regex("^\\.\\.\\.\\s+(\\w+)\\s+(\\d{1,3})");
            if (regex.indexIn(wide) != -1)
            {
                const auto done = regex.cap(2).toFloat();
                emit unpackProgress(done);
                DEBUG("emit signal!");
            }
            line.clear();
        }
        else if (byte == '\b')
            line.push_back('.');
        else line.push_back(byte);;
    }
    stdout_.chop(chop - line.size());
}


void Unpacker::processStdErr()
{
    Q_ASSERT(!"We should be using merged IO channels for stdout and stderr."
        "See the code in startNextRecovery()");
}

void Unpacker::processFinished(int exitCode, QProcess::ExitStatus status)
{
    const auto index = list_->findActive();
    if (index == UnpackList::NoSuchUnpack)
        return;

    processStdOut();

    auto& active = list_->getArchive(index);
    DEBUG("Unpack %1 finished. ExitCode %2, ExitStatus %3",
        active.desc, exitCode, status);

    if (status == QProcess::CrashExit)
    {
        active.state   = Archive::Status::Error;
        active.message = "Crash exit";
        ERROR("Unpack '%1' error %2", active.desc, active.message);
    }
    else
    {

    }
    list_->refresh(index);

    emit unpackReady(active);

    startNextUnpack();
}

void Unpacker::processError(QProcess::ProcessError error)
{
    DEBUG("ProcessError %1", error);

    const auto index = list_->findActive();
    if (index == UnpackList::NoSuchUnpack)
        return;

    auto& active = list_->getArchive(index);

    active.state   = Archive::Status::Error;
    active.message = "Process error";
    ERROR("Unpack '%1' error %2", active.desc, active.message);

    list_->refresh(index);

    unpackReady(active);

    startNextUnpack();

}

void Unpacker::startNextUnpack()
{
    const auto state = process_.state();
    if (state != QProcess::NotRunning)
        return;

    const auto index = list_->findNextPending();
    if (index == UnpackList::NoSuchUnpack)
    {
        DEBUG("All done!");
        emit allDone();
        return;
    }
    stdout_.clear();
    stderr_.clear();

    auto& arc = list_->getArchive(index);

    const auto& unrar = distdir::path("unrar");
    const auto& file  = arc.path + "/" + arc.file;

    QStringList args;
    args << "e";
    args << arc.file;
    process_.setWorkingDirectory(arc.path);
    process_.setProcessChannelMode(QProcess::MergedChannels);
    process_.start(unrar, args);
    DEBUG("Starting unpack for %1", arc.desc);

    arc.state = Archive::Status::Active;
    list_->refresh(index);

    emit unpackStart(arc);
}

Unpacker* g_unpacker;

} // app