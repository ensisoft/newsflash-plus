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
#include "archive.h"
#include "archiver.h"

namespace app
{

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

        if (role == Qt::DecorationRole)
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
        BOUNDSCHECK(list_, index);
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
                case Columns::File:   return data;
                case Columns::LAST:   Q_ASSERT(0);
            }
        }
        if (role == Qt::DecorationRole)
        {
            const static QIcon ico("icons:ico_bullet_green.png");
            return ico;
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

    void update(const app::Archive& arc, const QString& file)
    {
        Q_ASSERT(file.isEmpty() == false);

        const auto curSize = (int)files_.size();

        QAbstractTableModel::beginInsertRows(QModelIndex(), curSize, curSize);
        files_.push_back(file);
        QAbstractTableModel::endInsertRows();
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

private:
    friend class Unpacker;
    std::vector<QString> files_;
};


Unpacker::Unpacker(std::unique_ptr<Archiver> archiver) : engine_(std::move(archiver))
{
    DEBUG("Unpacker created");

    list_.reset(new UnpackList);    
    data_.reset(new UnpackData);

    engine_->onExtract = std::bind(&UnpackData::update, data_.get(),
        std::placeholders::_1, std::placeholders::_2);

    engine_->onProgress = [=](const app::Archive& arc, const QString& target, int done) {
        emit unpackProgress(target, done);
    };

    engine_->onReady = [=](const app::Archive& arc) {
        DEBUG("Archive %1 unpack ready with status %2", arc.file, toString(arc.state));

        if (arc.state == Archive::Status::Success)
        {
            NOTE("Archive %1 succesfully unpacked.", arc.desc);
            INFO("Archive %1 succesfully unpacked.", arc.desc);
        }
        else if (arc.state == Archive::Status::Failed)
        {
            WARN("Archive %1 unpack failed. %2", arc.desc, arc.message);
        }
        else if (arc.state == Archive::Status::Error)
        {
            ERROR("Archive %1 unpack error. %2", arc.desc, arc.message);
        }

        emit unpackReady(arc);

        const auto index = list_->findActive();
        if (index != UnpackList::NoSuchUnpack)
        {
            auto& active = list_->getArchive(index);
            active = arc;
            list_->refresh(index);
        }

        startNextUnpack();
    };

    enabled_    = true;
    cleanup_    = false;
    overwrite_  = false;
    keepBroken_ = true;
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
    engine_->stop();
}

void Unpacker::startNextUnpack()
{
    if (engine_->isRunning())
        return;

    const auto index = list_->findNextPending();
    if (index == UnpackList::NoSuchUnpack)
    {
        DEBUG("Unpacking all done!");
        return;
    }

    Archiver::Settings settings;
    settings.keepBroken        = keepBroken_;
    settings.purgeOnSuccess    = cleanup_;
    settings.overWriteExisting = overwrite_;

    auto& unpack = list_->getArchive(index);
    unpack.state = Archive::Status::Active;
    engine_->extract(unpack, settings);

    data_->clear();
    list_->refresh(index);

    emit unpackStart(unpack);

    DEBUG("Start unpack for %1", unpack.file);

}

QStringList Unpacker::findUnpackVolumes(const QStringList& fileEntries)
{
    return engine_->findArchives(fileEntries);
}

} // app