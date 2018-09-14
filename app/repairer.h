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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QAbstractTableModel>
#  include <QByteArray>
#  include <QObject>
#  include <QProcess>
#  include <QMetaType>
#include "newsflash/warnpop.h"
#include <memory>
#include <list>
#include "paritychecker.h"
#include "archive.h"

namespace app
{
    // perform recovery operation on a batch of files based on
    // the par2 recovery files. Listens for engine signals
    // and creates new recovery operations when applicable
    // automatically.
    class Repairer : public QObject
    {
        Q_OBJECT

    public:
        Repairer(std::unique_ptr<ParityChecker> engine);
       ~Repairer();

        // get a table model for the detail file
        // par2 file data for the current recovery process.
        QAbstractTableModel* getRecoveryData();

        // get a table model for a specific recovery.
        QAbstractTableModel* getRecoveryData(const QModelIndex& index);

        // get a table model to see the current recovery list.
        QAbstractTableModel* getRecoveryList();


        // add a new recovery to be performed.
        // the recovery will be in queued state after this.
        void addRecovery(const Archive& arc);

        // stop current recovery operation.
        // this will run the scheduler and start the next
        // recovery operation if any.
        // will also invoke the repairReady signal.
        void stopRecovery();

        // shutdown the repairer. will stop any pending
        // recovery operation without invoking any signals.
        // this should be called when the application is
        // shutting down.
        void shutdown();

        void moveUp(QModelIndexList& list);

        void moveDown(QModelIndexList& list);

        void moveToTop(QModelIndexList& list);

        void moveToBottom(QModelIndexList& list);

        void kill(QModelIndexList& list);

        void killComplete();

        void openLog(QModelIndexList& list);

        void setWriteLogs(bool onOff)
        { mWriteLogs = onOff; }
        void setPurgePars(bool onOff)
        { mPurgePars = onOff; }

        std::size_t numRepairs() const;

        const Archive& getRecovery(const QModelIndex&) const;

        void setEnabled(bool onOff)
        {
            mEnabled = onOff;
            if (mEnabled)
                startNextRecovery();
        }

        bool isEnabled() const
        { return mEnabled; }

    signals:
        void repairEnqueue(const app::Archive& arc);
        void repairStart(const app::Archive& rec);
        void repairReady(const app::Archive& rec);
        void scanProgress(const QString& file, int val);
        void repairProgress(const QString& step, int val);

    private:
        void startNextRecovery();

    private:
        class RecoveryData;
        class RecoveryList;
        std::unique_ptr<RecoveryData> mData;
        std::unique_ptr<RecoveryData> mInfo;
        std::unique_ptr<RecoveryList> mList;
        std::unique_ptr<ParityChecker> mEngine;

        struct RecoveryFiles {
            quint32 archiveID;
            std::vector<ParityChecker::File> files;
        };
        std::list<RecoveryFiles> mHistory;

    private:
        bool mWriteLogs = true;
        bool mPurgePars = false;
        bool mEnabled   = true;
    };

} // app

