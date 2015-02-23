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
#include <deque>

namespace app
{
    struct Archive {
        enum class Status {
            // archive is queued and will execute at some later time.
            Queued,

            // archive is currently being unpacked.
            Active,

            // archive was succesfully unpacked.
            Success, 

            // archive failed to unpack
            Failed,

            // error running the unpack operation.
            Error
        };

        // current unpack state of the archive.
        Status state;

        // path to the unpack source folder (where the .rar files are)
        QString path;

        // the main .rar file
        QString file;

        // human readable description.
        QString desc;

        // error/information message
        QString message;
    };

    // unpacker unpacks archive files such as .rar. 
    class Unpacker : public QObject 
    {
        Q_OBJECT

    public:
        Unpacker();
       ~Unpacker();

        // get unpack list data model
        QAbstractTableModel* getUnpackList();

        QAbstractTableModel* getUnpackData();

        // add a new unpack operation to be performed. 
        void addUnpack(const Archive& arc);

        // stop current unpack operation.
        void stopUnpack();

    signals:
        void allDone();
        void unpackStart(const app::Archive& arc);
        void unpackReady(const app::Archive& arc);
        void unpackProgress(int done);

    private slots:
        void processStdOut();
        void processStdErr();
        void processFinished(int exitCode, QProcess::ExitStatus status);
        void processError(QProcess::ProcessError error);        

    private:
        void startNextUnpack();

    private:
        QProcess process_;
        QByteArray stdout_;
        QByteArray stderr_;
    private:
        class UnpackList;
        class UnpackData;
        std::unique_ptr<UnpackList> list_;
        std::unique_ptr<UnpackData> data_;
    };

    extern Unpacker* g_unpacker;

} // app

    Q_DECLARE_METATYPE(app::Archive);