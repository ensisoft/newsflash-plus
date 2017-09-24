// Copyright (c) 2010-2017 Sami Väisänen, Ensisoft
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

#define LOGTAG "process"


#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QTextStream>
#include "newsflash/warnpop.h"

#include <cctype>
#include <string>

#include "engine/assert.h"
#include "eventlog.h"
#include "debug.h"
#include "format.h"
#include "process.h"

namespace {

void parseLines(QByteArray& buff, QStringList& lines)
{
    std::string temp;

    for (int i=0; i<buff.size(); ++i)
    {
        const auto byte = buff.at(i);
        if (byte == '\n')
        {
            const auto line = QString::fromStdString(temp);
            lines << line;
            temp.clear();
        }
        else if (byte == '\b')
        {
            temp.push_back('.');
        }
        else if (!std::iscntrl((unsigned)byte))
        {
            temp.push_back(byte);
        }
    }
    const auto leftOvers = temp.size();
    buff = buff.right(leftOvers);
}

} // namespace

namespace app
{

Process::Process()
{
    QObject::connect(&mProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
        this, SLOT(processFinished(int, QProcess::ExitStatus)));
    QObject::connect(&mProcess, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(processError(QProcess::ProcessError)));
    QObject::connect(&mProcess, SIGNAL(stateChanged(QProcess::ProcessState)),
        this, SLOT(processState(QProcess::ProcessState)));
    QObject::connect(&mProcess, SIGNAL(readyReadStandardOutput()),
        this, SLOT(processStdOut()));
    QObject::connect(&mProcess, SIGNAL(readyReadStandardError()),
        this, SLOT(processStdErr()));
}

Process::~Process()
{
    const auto state = mProcess.state();
    ASSERT(state == QProcess::NotRunning
        && "Process is still running. It should be either stopped or waited to complete.");
    Q_UNUSED(state);
}

void Process::start(const QString& executable,
                    const QStringList& args,
                    const QString& logFile,
                    const QString& workingDir)
{
    ASSERT(mProcess.state() == QProcess::NotRunning);
    ASSERT(!mLogFile.isOpen());

    mStdOut.clear();
    mStdErr.clear();
    mError = Error::None;
    if (!logFile.isEmpty())
    {
        mLogFile.setFileName(logFile);
        mLogFile.open(QIODevice::Append | QIODevice::WriteOnly);
        if (!mLogFile.isOpen())
        {
            WARN("Unable to write log file %1, %2", logFile, mLogFile.error());
        }
    }

    mExecutable = executable;
    mWorkingDir = workingDir;

    if (!workingDir.isEmpty())
        mProcess.setWorkingDirectory(workingDir);
    mProcess.setProcessChannelMode(QProcess::SeparateChannels);
    mProcess.start(executable, args);
}

void Process::kill()
{
    if (mProcess.state() == QProcess::Running)
    {
        // terminate will ask the process nicely to exit
        // QProcess::kill will just kill it forcefully.
        // looks like terminate will send sigint and unrar obliges and exits cleanly.
        // however this means that process's return state is normal exit.
        // whereas unrar just dies.
        mProcess.kill();
        if (mLogFile.isOpen())
        {
            // dump the last buffer bits into log.
            mLogFile.write(mStdOut);
            mLogFile.write("\n");
            mLogFile.write(mStdErr);
            mLogFile.write("\n");
            mLogFile.write("*** terminated by user ***");
            mLogFile.flush();
            mLogFile.close();
        }
    }
}

// static
bool Process::runAndCapture(const QString& executable,
    QStringList& stdout,
    QStringList& stderr)
{
    Process process;
    process.onStdErr = [&](const QString& line) {
        stderr.append(line);
    };
    process.onStdOut = [&](const QString& line) {
        stdout.append(line);
    };

    const auto& logFile    = QString("");
    const auto& workingDir = QString("");
    process.start(executable, QStringList(), logFile, workingDir);

    const auto NeverTimeout = -1;
    process.mProcess.waitForFinished(NeverTimeout);
    if (process.error() != Error::None)
        return false;

    return true;
}

void Process::processStdOut()
{
    const QByteArray& buff = mProcess.readAllStandardOutput();
    mStdOut.append(buff);
    if (mStdOut.isEmpty())
        return;

    QStringList lines;
    parseLines(mStdOut, lines);

    QTextStream stream(&mLogFile);
    stream.setCodec("UTF-8");

    for (const auto& line : lines)
    {
        if (onStdOut)
            onStdOut(line);
        if (mLogFile.isOpen())
            stream << line << "\r\n";
    }
    if (mLogFile.isOpen())
        mLogFile.flush();
}

void Process::processStdErr()
{
    const QByteArray& buff = mProcess.readAllStandardError();
    mStdErr.append(buff);
    if (mStdErr.isEmpty())
        return;

    QStringList lines;
    parseLines(mStdErr, lines);

    QTextStream stream(&mLogFile);
    stream.setCodec("UTF-8");

    for (const auto& line : lines)
    {
        if (onStdErr)
            onStdErr(line);
        if (mLogFile.isOpen())
            stream << line << "\r\n";
    }
    if (mLogFile.isOpen())
        mLogFile.flush();
}

void Process::processFinished(int exitCode, QProcess::ExitStatus status)
{
    DEBUG("%1 finished exitCode: %2 exitStatus: %3",
        mExecutable, exitCode, status);

    processStdOut();
    processStdErr();

    if (mLogFile.isOpen())
    {
        mLogFile.flush();
        mLogFile.close();
    }
    if (onFinished)
        onFinished();
}

void Process::processError(QProcess::ProcessError error)
{
    DEBUG("%1 error %2", mExecutable, error);
    ERROR("%1 error %2", mExecutable, error);

    // Umh.. Qt's QProcessError doesn't have a "no error" enum.
    // so apprently we don't really know if an error happened
    // unless we set a flag in this signal
    switch (error)
    {
        case QProcess::FailedToStart:
            mError = Error::FailedToStart;
            break;
        case QProcess::Crashed:
            mError = Error::Crashed;
            break;
        case QProcess::Timedout:
            mError = Error::Timedout;
            break;
        case QProcess::ReadError:
            mError = Error::ReadError;
            break;
        case QProcess::WriteError:
            mError = Error::WriteError;
        case QProcess::UnknownError:
            mError = Error::OtherError;
            break;
    }
    if (mLogFile.isOpen())
    {
        mLogFile.write(mStdOut);
        mLogFile.write("\r\n");
        mLogFile.write(mStdErr);
        mLogFile.write("\r\n");
        mLogFile.write("*** process error ***");
        mLogFile.flush();
    }
}

void Process::processState(QProcess::ProcessState state)
{
    DEBUG("%1 state %2", mExecutable, state);
}

} // namespace

