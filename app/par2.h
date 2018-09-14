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
#  include <QString>
#include "newsflash/warnpop.h"

#include "paritychecker.h"
#include "parstate.h"
#include "archive.h"
#include "process.h"

namespace app
{
    // implementation of parity checking using par2 command line utiliy.
    class Par2 : public ParityChecker
    {
    public:
        Par2(const QString& executable);
       ~Par2();

        // ParityChecker implementation
        virtual void recover(const Archive& arc, const Settings& s) override;
        virtual void stop() override;
        virtual bool isRunning() const override;
        virtual bool getCurrentArchiveData(Archive* arc) const override;

        static QStringList getCopyright(const QString& executable);

    private:
        void onFinished();
        void onStdErr(const QString& line);
        void onStdOut(const QString& line);

    private:
        Process mProcess;
        QString mPar2Executable;
    private:
        Archive mCurrentArchive;
    private:
        ParState mParState;
        QString  mErrors;
    };
} // app
