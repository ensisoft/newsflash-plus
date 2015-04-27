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

#pragma once

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QAbstractTableModel>
#  include <QByteArray>
#  include <QObject>
#  include <QProcess>
#  include <QStringList>
#include <newsflash/warnpop.h>
#include <memory>
#include "archiver.h"

namespace app
{
    struct Archive;
    class Archiver;

    // unpacker unpacks archive files such as .rar. 
    class Unpacker : public QObject
    {
        Q_OBJECT

    public:
        Unpacker(std::unique_ptr<Archiver> archiver);
       ~Unpacker();

        // get unpack list data model
        QAbstractTableModel* getUnpackList();

        QAbstractTableModel* getUnpackData();

        // add a new unpack operation to be performed. 
        void addUnpack(const Archive& arc);

        // stop current unpack operation.
        void stopUnpack();

        void moveUp(QModelIndexList& list);

        void moveDown(QModelIndexList& list);

        void moveToTop(QModelIndexList& list);

        void moveToBottom(QModelIndexList& list);

        void kill(QModelIndexList& list);

        void killComplete();

        void openLog(QModelIndexList& list);

        void setEnabled(bool onOff)
        { 
            enabled_ = onOff; 
            if (enabled_)
                startNextUnpack();
        }

        void setPurgeOnSuccess(bool onOff)
        { cleanup_ = onOff; }

        void setKeepBroken(bool onOff)
        { keepBroken_ = onOff; }

        void setOverwriteExisting(bool onOff)
        { overwrite_ = onOff; }

        void setWriteLog(bool onOff)
        { writeLog_ = onOff; }

        bool isEnabled() const 
        { return enabled_; }

        QStringList findUnpackVolumes(const QStringList& fileEntries);

        const Archive& getUnpack(const QModelIndex& index) const;

        std::size_t numUnpacks() const ;

    signals:
        void unpackEnqueue(const app::Archive& arc);
        void unpackStart(const app::Archive& arc);
        void unpackReady(const app::Archive& arc);
        void unpackProgress(const QString& file, int done);

    private:
        void startNextUnpack();

    private:
        class UnpackList;
        class UnpackData;
        std::unique_ptr<UnpackList> list_;
        std::unique_ptr<UnpackData> data_;
        std::unique_ptr<Archiver> engine_;
    private:
        bool enabled_;
        bool cleanup_;
        bool overwrite_;
        bool keepBroken_;
        bool writeLog_;
    };

} // app

