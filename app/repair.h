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
#  include <QMetaType>
#include <newsflash/warnpop.h>
#include <memory>
#include "engine.h"

namespace app
{
    struct Recovery {
        enum class Status {
            // recovery is currently queued and will execute
            // at some later time.
            Queued,  

            // recovery is currently being performed.
            // recovery data will show the data
            // inside these parity files.
            Active, 

            // recovery was succesfully performed.
            Success, 

            // recovery failed
            Failed,

            // error running the recovery operation
            Error
        };

        // current recovery status.
        Status  state;

        // path to the recovery folder (where the par2 and data files are)
        QString path;

        // the main par2 file
        QString file; 

        // human readable description
        QString desc;

        QString message;
    };


    // perform recovery operation on a batch of files based on
    // the par2 recovery files. Listens for engine signals
    // and creates new recovery operations when applicable 
    // automatically.
    class RepairEngine : public QObject 
    {
        Q_OBJECT

    public:
        RepairEngine();
       ~RepairEngine();

        // get a table model for the detail file
        // par2 file data for the current recovery process.
        QAbstractTableModel* getRecoveryData();

        // get a table model to see the current recovery list.
        QAbstractTableModel* getRecoveryList();

        // add a new recovery to be performed.
        // the recovery will be in queued state after this.
        void addRecovery(const Recovery& rec);


        void stopRecovery();

    signals:
        void allDone();
        void recoveryStart(const app::Recovery& rec);
        void recoveryReady(const app::Recovery& rec);

        void scanProgress(const QString& file, int val);
        void repairProgress(const QString& step, int val);                

    private slots:
        void processStdOut();
        void processStdErr();
        void processFinished(int exitCode, QProcess::ExitStatus status);
        void processError(QProcess::ProcessError error);
        void fileCompleted(const app::DataFileInfo& info);
        void batchCompleted(const app::FileBatchInfo& info);
    private:
        void startNextRecovery();

    private:
        QProcess process_;
        QByteArray stdout_;
        QByteArray stderr_;
    private:
        class RecoveryData;
        class RecoveryList;        
        std::unique_ptr<RecoveryData> data_;
        std::unique_ptr<RecoveryList> list_;
    };

    extern RepairEngine* g_repair;



} // app

    Q_DECLARE_METATYPE(app::Recovery);