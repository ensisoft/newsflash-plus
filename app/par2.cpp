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

#define LOGTAG "par2"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QStringList>
#include "newsflash/warnpop.h"

#include <string>

#include "eventlog.h"
#include "format.h"
#include "debug.h"
#include "par2.h"
#include "utility.h"

namespace app
{

Par2::Par2(const QString& executable) : mPar2Executable(executable)
{
    mProcess.onFinished = std::bind(&Par2::onFinished, this);
    mProcess.onStdErr   = std::bind(&Par2::onStdErr, this,
        std::placeholders::_1);
    mProcess.onStdOut   = std::bind(&Par2::onStdOut, this,
        std::placeholders::_1);
}


void Par2::recover(const Archive& arc, const Settings& s)
{

    const QString& logFile = s.writeLogFile
        ? app::joinPath(arc.path, "repair.log")
        : "";
    const QString& workingDir = arc.path;

    QStringList args;
    args << "r";
    if (s.purgeOnSuccess)
        args << "-p";

    args << arc.file;
    args << "./*"; // scan extra files

    // important. we must set the current_ *before* calling start()
    // since start can emit signals synchronously from the call to start
    // when the executable fails to launch. Nice semantic change there!
    mCurrentArchive = arc;
    mProcess.start(mPar2Executable, args, logFile, workingDir);

    DEBUG("Started par2 for %1", arc.file);
}

void Par2::stop()
{
    DEBUG("Killing par2 of archive %1", mCurrentArchive.file);

    mCurrentArchive.state = Archive::Status::Stopped;

    mProcess.kill();
}

bool Par2::isRunning() const
{
    return mProcess.isRunning();
}

void Par2::onStdOut(const QString& line)
{
    const auto exec = mParState.getState();

    ParityChecker::File file;
    if (mParState.update(line, file))
    {
        onUpdateFile(mCurrentArchive, std::move(file));
    }

    if (exec == ParState::ExecState::Scan)
    {
        QString file;
        float done = 0.0f;
        if (ParState::parseScanProgress(line, file, done))
        {
            onScanProgress(mCurrentArchive, file, done);
        }
    }
    else if (exec == ParState::ExecState::Repair)
    {
        QString step;
        float done = 0.0f;
        if (ParState::parseRepairProgress(line, step, done))
        {
            onRepairProgress(mCurrentArchive, step, done);
        }
    }
}

void Par2::onStdErr(const QString& line)
{
    // currently all handling is intentionally in the stdOut handler.
    onStdOut(line);
}

void Par2::onFinished()
{
    DEBUG("par2 finished. Archive %1", mCurrentArchive.file);

    if (mCurrentArchive.state != Archive::Status::Stopped)
    {
        const auto error = mProcess.error();
        if (error == Process::Error::None)
        {
            const auto message = mParState.getMessage();
            const auto success = mParState.getSuccess();
            mCurrentArchive.message = message;
            if (success)
            {
                INFO("Par2 %1 success", mCurrentArchive.file);
                mCurrentArchive.state = Archive::Status::Success;
            }
            else
            {
                WARN("Par2 %1 failed", mCurrentArchive.file);
                mCurrentArchive.state = Archive::Status::Failed;
            }
        }
        else if (error == Process::Error::Crashed)
        {
            ERROR("Par2 process crashed when repairing %1", mCurrentArchive.file);
            mCurrentArchive.message = "Crashed :(";
            mCurrentArchive.state   = Archive::Status::Error;
        }
        else
        {
            ERROR("Par2 process failed to run on %1", mCurrentArchive.file);
            mCurrentArchive.message = "Unknown process error :(";
            mCurrentArchive.state   = Archive::Status::Error;
        }
    }
    onReady(mCurrentArchive);
}

// static
QStringList Par2::getCopyright(const QString& executable)
{
    QStringList ret;
    QStringList stdout;
    QStringList stderr;

    QStringList args;
    args << "-VV"; // for version and copyright
    if (Process::runAndCapture(executable, stdout, stderr, args))
    {
        for (const auto& line : stdout)
        {
            const auto& LINE = line.toUpper();
            if (LINE.contains("COPYRIGHT"))
                ret << line;
            else if (LINE.contains("VERSION") && LINE.contains("PAR2CMDLINE"))
                ret << line;
        }
    }
    return ret;
}

} // app
