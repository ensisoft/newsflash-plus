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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QRegExp>
#  include <QtDebug>
#include <newsflash/warnpop.h>
#include <string>
#include "unrar.h"
#include "debug.h"
#include "format.h"

namespace {

bool isMatch(const QString& line, const QString& test, QStringList& captures, int numCaptures)
{
    QRegExp exp;
    exp.setPattern(test);
    exp.setCaseSensitivity(Qt::CaseInsensitive);
    Q_ASSERT(exp.isValid());

    if (exp.indexIn(line) == -1)
        return false;

    Q_ASSERT(exp.captureCount() == numCaptures);

    // cap(0) is the whole regexp

    captures.clear();
    for (int i=0; i<numCaptures; ++i)
    {
        captures << exp.cap(i+1);
        //qDebug() << exp.cap(i+1);
    }

    return true;
}

} // namespace

namespace app
{

Unrar::Unrar(const QString& executable) : unrar_(executable)
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

Unrar::~Unrar()
{
    const auto state = process_.state();
    Q_ASSERT(state == QProcess::NotRunning &&
        "Current archive is still being processed."
        "We should either wait for its completion or stop it");        
}

void Unrar::extract(const Archive& arc, const Settings& settings)
{
    const auto state = process_.state();
    Q_ASSERT(state == QProcess::NotRunning &&
        "We already have a current archive being extracted");

    stdout_.clear();
    stderr_.clear();
    message_.clear();
    success_  = false;
    finished_ = false;

    QStringList args;
    args << "e"; // for extract
    args << "-o+";
    args << arc.file;
    process_.setWorkingDirectory(arc.path);
    process_.setProcessChannelMode(QProcess::MergedChannels);
    process_.start(unrar_, args);
    archive_ = arc;
    DEBUG("Started unrar for %1", arc.file);
}

void Unrar::stop()
{
    const auto state = process_.state();
    if (state == QProcess::NotRunning)
        return;

    DEBUG("Killing unrar %1, pid %2", unrar_, process_.pid());

    archive_.state = Archive::Status::Stop;

    // terminate will ask the process nicely to exit
    // QProcess::kill will just kill it forcefully.
    // looks like terminate will send sigint and unrar obliges and exits cleanly.
    // however this means that process's return state is normal exit. 
    // whereas unrar just dies.
    process_.kill();
}

bool Unrar::isRunning() const 
{
    const auto state = process_.state();
    if (state == QProcess::NotRunning)
        return false;

    return true;
}

bool Unrar::parseVolume(const QString& line, QString& volume)
{
    QStringList captures;
    if (isMatch(line, "Extracting from (.+)", captures, 1))
    {
        volume = captures[0];
        return true;
    }
    return false;
}

bool Unrar::parseProgress(const QString& line, QString& file, int& done)
{
    QStringList captures;
    if (isMatch(line, "\\s+(\\S+)\\D+(\\d+)", captures, 2))
    {
        file = captures[0];
        done = captures[1].toInt();
        return true;
    }
    return false;
}

bool Unrar::parseTermination(const QString& line, QString& message, bool& success)
{
    QStringList captures;
    if (isMatch(line, "(All OK)", captures, 1))
    {
        message = captures[0];
        success = true;
        return true;
    }
    else if (isMatch(line, "(ERROR: .+)", captures, 1))
    {
        message = captures[0];
        success = false;
        return true;
    }
    return false;
}

void Unrar::processStdOut()
{
    if (finished_) return;

    const auto buff = process_.readAllStandardOutput();
    const auto size = buff.size();
    //DEBUG("Unrar read %1 bytes", size);

    stdout_.append(buff);

    std::string temp;
    for (int i=0; i<stdout_.size(); ++i)
    {
        const auto byte = stdout_.at(i); 
        if (byte == '\n')
        {
            const auto line = widen(temp);
            if (line.isEmpty())
                continue;

            QString str;
            int done;
            if (parseVolume(line, str))
            {
                onExtract(archive_, str);
            }
            else if (parseProgress(line, str, done))
            {
                onProgress(archive_, str, done);
            }
            else if (parseTermination(line, message_, success_))
            {
                DEBUG("unrar terminated. %1", message_);
                finished_ = true;
                return;
            }

            temp.clear();
        }
        else if (byte == '\b')
        {
            temp.push_back('.');
        }
        else temp.push_back(byte);
    }
    const auto leftOvers = (int)temp.size();
    stdout_ = stdout_.right(leftOvers);
}

void Unrar::processStdErr()
{
    Q_ASSERT(!"We should be using merged IO channels for stdout and stderr."
        "See the code in startNextRecovery()");
}


void Unrar::processFinished(int exitCode, QProcess::ExitStatus status)
{
    DEBUG("unrar %1 finished. ExitCode %2, ExitStatus %3", 
        unrar_, exitCode, status);

    if (archive_.state != Archive::Status::Stop)
    {
        if (status == QProcess::CrashExit)
        {
            archive_.message = toString(status);
            archive_.state   = Archive::Status::Error;
        }
        else
        {
            processStdOut();

            DEBUG("unrar result %1 message %2", success_, message_);
            archive_.message = message_;
            archive_.state   = success_ ? Archive::Status::Success :
                Archive::Status::Failed;
        }
    }
    onReady(archive_);
}

void Unrar::processError(QProcess::ProcessError error)
{
    DEBUG("Unrar %1 process error %1", unrar_, error);

    archive_.message = toString(error);
    archive_.state   = Archive::Status::Error;

    onReady(archive_);
}

void Unrar::processState(QProcess::ProcessState state)
{
    DEBUG("unrar state %1 pid %2", state, process_.pid());
}

} // app