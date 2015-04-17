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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QStringList>
#include <newsflash/warnpop.h>
#include <string>
#include "eventlog.h"
#include "format.h"
#include "debug.h"
#include "par2.h"

namespace app
{

Par2::Par2(const QString& executable) : par2_(executable)
{
    QObject::connect(&process_, SIGNAL(finished(int, QProcess::ExitStatus)), 
        this, SLOT(processFinished(int, QProcess::ExitStatus)));
    QObject::connect(&process_, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(processError(QProcess::ProcessError)));
    QObject::connect(&process_, SIGNAL(stateChanged(QProcess::ProcessState)),
        this, SLOT(processState(QProcess::ProcessState)));
    QObject::connect(&process_, SIGNAL(readyReadStandardOutput()),
        this, SLOT(processStdOut()));
    QObject::connect(&process_, SIGNAL(readyReadStandardError()),
        this, SLOT(processStdErr()));        
}

Par2::~Par2()
{
    const auto state = process_.state();
    Q_ASSERT(state == QProcess::NotRunning && 
        "Current archive is still being processed."
        "We should either wait for its completion or stop it.");
}

void Par2::recover(const Archive& arc, const Settings& s)
{
    const auto state = process_.state();
    Q_ASSERT(state == QProcess::NotRunning &&
        "we already have a current recovery being processed.");

    stdout_.clear();
    stderr_.clear();
    state_.clear();

    if (s.writeLogFile)
    {
        const auto path = arc.path;
        const auto file = path + "/repair.log";
        logFile_.setFileName(file);
        logFile_.open(QIODevice::Append | QIODevice::WriteOnly);
        if (!logFile_.isOpen())
        {
            WARN("Unable to write par2 log file %1, %2", file, logFile_.error());
        }
    }

    QStringList args;
    args << "r";
    if (s.purgeOnSuccess)
        args << "-p";

    args << arc.file;
    args << "./*"; // scan extra files

    // important. we must set the current_ *before* calling start()
    // since start can emit signals synchronously from the call to start
    // when the executable fails to launch. Nice semantic change there!
    current_ = arc;
    process_.setWorkingDirectory(arc.path);
    process_.setProcessChannelMode(QProcess::MergedChannels);
    process_.start(par2_, args);

    DEBUG("Started par2 %1, %2", par2_, arc.file);
}

void Par2::stop()
{
    const auto state = process_.state();
    if (state == QProcess::NotRunning)
        return;

    DEBUG("Killing par2 %1, pid %2", par2_, process_.pid());

    current_.state = Archive::Status::Stopped;

    // this will ask the process to terminate nicely
    // QProcess::kill will just terminate it right away.
    process_.terminate();

    logFile_.write(stdout_);
    logFile_.write("*** Terminated by user. ***");
    logFile_.close();        
}

bool Par2::isRunning() const 
{
    const auto state = process_.state();
    if (state == QProcess::NotRunning)
        return false;
    return true;
}

void Par2::processStdOut()
{
    const auto buff = process_.readAllStandardOutput();
    const auto size = buff.size();
    //DEBUG("par2 %1 wrote %2 bytes", par2_, size);

    stdout_.append(buff);

    std::string temp;
    for (int i=0; i<stdout_.size(); ++i)
    {
        const auto byte = stdout_.at(i);
        if (byte == '\r' || byte == '\n')
        {
            const auto line = widen(temp);
            const auto exec = state_.getState();
            ParityChecker::File file;
            if (state_.update(line, file))
            {
                onUpdateFile(current_, std::move(file));
                if (byte == '\n')
                {
                    if (logFile_.isOpen())
                    {
                        logFile_.write(temp.data());
                        logFile_.write("\n");
                    }
                }
            }
            if (exec == ParState::ExecState::Scan)
            {
                QString file;
                float done;
                if (ParState::parseScanProgress(line, file, done))
                    onScanProgress(current_, file, done);
            }
            if (exec == ParState::ExecState::Repair)
            {
                QString step;
                float done;
                if (ParState::parseRepairProgress(line, step, done))
                    onRepairProgress(current_, step, done);
            }
            temp.clear();
        }
        else temp.push_back(byte);
    }
    const auto leftOvers = (int)temp.size();
    stdout_ = stdout_.right(leftOvers);
}

void Par2::processStdErr()
{
    Q_ASSERT(!"We should be using merged IO channels for stdout and stderr.");
}

void Par2::processFinished(int exitCode, QProcess::ExitStatus status)
{
    DEBUG("par2 %1 finished. Exitcode %2, ExitStatus %3", 
        par2_, exitCode, status);

    if (current_.state != Archive::Status::Stopped)
    {
        if (status == QProcess::CrashExit)
        {
            current_.message = toString(status);
            current_.state   = Archive::Status::Error;
            if(logFile_.isOpen())
            {
                logFile_.write("*** Crash Exit ***");
                logFile_.flush();
                logFile_.close();
            }
        }
        else
        {
            // deal with any remaining output 
            processStdOut();
        
            const auto state   = state_.getState();
            const auto message = state_.getMessage();
            const auto success = state_.getSuccess();
            DEBUG("par2 result %1 message %2", success, message);

            current_.message = message;
            current_.state   = success ? Archive::Status::Success : 
                Archive::Status::Failed;

            if (logFile_.isOpen())
            {
                const auto msg = toLatin(message);
                logFile_.write(msg.data());
                logFile_.write("\n");
            }
        }
    }
    if (logFile_.isOpen())
    {
        logFile_.flush();
        logFile_.close();
    }
    onReady(current_);
}


void Par2::processError(QProcess::ProcessError error)
{
    DEBUG("par2 %1 process error. %2", par2_, error);

    current_.message = toString(error);
    current_.state   = Archive::Status::Error;

    if  (logFile_.isOpen())
    {
        logFile_.write(stdout_);
        logFile_.write("*** Process Error ***");
        logFile_.flush();
        logFile_.close();
    }

    onReady(current_);
}

void Par2::processState(QProcess::ProcessState state)
{
    DEBUG("par2 state %1 pid %2", state, process_.pid());
}

} // app
