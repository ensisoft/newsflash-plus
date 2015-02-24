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
#  include <QObject>
#  include <QProcess>
#  include <QByteArray>
#include <newsflash/warnpop.h>
#include "paritychecker.h"
#include "parstate.h"
#include "archive.h"

namespace app
{
    // implementation of parity checking using par2 command line utiliy.
    class Par2 : public QObject, public ParityChecker
    {
        Q_OBJECT

    public:
        Par2(const QString& executable);
       ~Par2();

        virtual void recover(const Archive& arc, const Settings& s);
        virtual void stop();
        virtual bool isRunning() const;

    private slots:
        void processStdOut();
        void processStdErr();
        void processFinished(int exitCode, QProcess::ExitStatus status);
        void processError(QProcess::ProcessError error);
        void processState(QProcess::ProcessState state);

    private:
        QString par2_;
        QProcess process_;
        QByteArray stdout_;
        QByteArray stderr_;
    private:
        Archive current_;
    private:
        ParState state_;
    };
} // app