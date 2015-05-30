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

#define LOGTAG "unrar"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QRegExp>
#  include <QtDebug>
#include <newsflash/warnpop.h>
#include <string>
#include <cstring>
#include <map>
#include "unrar.h"
#include "debug.h"
#include "format.h"
#include "eventlog.h"

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
    Q_UNUSED(state);
}

void Unrar::extract(const Archive& arc, const Settings& settings)
{
    const auto state = process_.state();
    Q_ASSERT(state == QProcess::NotRunning &&
        "We already have a current archive being extracted");
    Q_UNUSED(state);

    stdout_.clear();
    stderr_.clear();
    message_.clear();
    errors_.clear();        
    files_.clear();
    success_  = true;
    cleanup_  = settings.purgeOnSuccess;

    if (settings.writeLog)
    {
        const auto path = arc.path;
        const auto file = path + "/extract.log";
        logFile_.close();
        logFile_.setFileName(file);
        logFile_.open(QIODevice::Append | QIODevice::WriteOnly);
        if (!logFile_.isOpen())
        {
            WARN("Unable to write unrar log file %1 %2", file, logFile_.error());
        }
    }

    QStringList args;
    args << "e"; // for extract

    if (settings.keepBroken)
        args << "-kb"; // keep broken
    if (settings.overWriteExisting)
        args << "-o+"; // enable overwrite.
    else args << "-or"; // rename automatically

    args << arc.file;

    // important. we must set the current_ *before* calling start()
    // since start can emit signals synchronously from the call to start
    // when the executable fails to launch. Nice semantic change there!
    archive_ = arc;

    process_.setWorkingDirectory(arc.path);
    process_.setProcessChannelMode(QProcess::SeparateChannels);
    process_.start(unrar_, args);
    
    DEBUG("Started unrar for %1", arc.file);
}

void Unrar::stop()
{
    const auto state = process_.state();
    if (state == QProcess::NotRunning)
        return;

    DEBUG("Killing unrar %1, pid %2", unrar_, process_.pid());

    archive_.state = Archive::Status::Stopped;

    // terminate will ask the process nicely to exit
    // QProcess::kill will just kill it forcefully.
    // looks like terminate will send sigint and unrar obliges and exits cleanly.
    // however this means that process's return state is normal exit. 
    // whereas unrar just dies.
    process_.kill();

    if (logFile_.isOpen())
    {
        logFile_.write(stdout_);
        logFile_.write(stderr_);
        logFile_.write("*** Terminated by user. ***");
        logFile_.close();
    }
}

bool Unrar::isRunning() const 
{
    const auto state = process_.state();
    if (state == QProcess::NotRunning)
        return false;

    return true;
}

QStringList Unrar::findArchives(const QStringList& fileNames) const 
{
    return Unrar::findVolumes(fileNames);
}

    // \D matches a non-digit
    // \d matches a digit
    // \S matches non-whitespace character
    // \s matches white space character

bool Unrar::parseMessage(const QString& line, QString& msg)
{
    QStringList captures;
    if (isMatch(line, "(All OK)", captures, 1))
    {
        msg = captures[0];
        return true;
    }
    else if (isMatch(line, "(.* is not RAR archive)", captures, 1))
    {
        msg = captures[0];
        return true;
    }
    return false;
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
    if (!isMatch(line, "(\\d+)%$", captures, 1))
        return false;

    QString str;
    const auto slash = line.lastIndexOf('/');
    if (slash != -1)
    {
        str = line.mid(slash + 1);
    }
    else
    {
        int mid = 0;
        for (;  mid< line.size(); ++mid)
        {
            if (!(line[mid] == ' ' || line[mid] == '.'))
                break;
        }
        str = line.mid(mid);
    }

    int chop = str.size() -1 ;
    for (; chop >= 0; chop--)
    {
        const auto next = str[chop];
        if (!(next == ' ' || next == '.' || next == '%' || next.isDigit()))
            break;
    }
    chop = str.size() - (chop + 1);
    str.chop(chop);

    done = captures[0].toInt();
    file = str;
    return true;
}

bool Unrar::parseTermination(const QString& line)
{
    QStringList captures;
    if (isMatch(line, "(All OK)", captures, 1))
    {
        return true;
    }

    return false;
}

QStringList Unrar::findVolumes(const QStringList& files)
{
    static const char* patterns[] = {
        ".part01.rar",
        ".part001.rar",
        ".r00",
    };

    std::map<QString, QString> s;

    for (const auto& pattern : patterns)
    {
        for (auto it = std::begin(files); it != std::end(files); )
        {
            it = std::find_if(it, std::end(files),
                [&](const QString& f) {
                    return f.endsWith(pattern);
                });
            if (it == std::end(files))
                continue;

            auto volume = *it;
            volume.chop(std::strlen(pattern));

            if (s.find(volume) == std::end(s))
            {
                s.insert({volume, *it});
            }
            ++it;
        }
    }

    for (auto it = std::begin(files); it != std::end(files);)
    {
        it = std::find_if(it, std::end(files), 
            [](QString f) {
                if (!f.endsWith(".rar"))
                    return false;

                f.chop(4);

                const QRegExp reg("\\.part\\d+$");
                if (reg.indexIn(f) != -1)
                    return false;

                return true;
            });
        if (it == std::end(files))
            continue;

        auto volume = *it;
        volume.chop(4);

        // potentially overwrite a previous .part01 or .r00 selection.
        s[volume] = *it;

        ++it;
    }

    // flatten
    QStringList ret;
    for (const auto& p : s)
        ret << p.second;

    return ret;
}

void Unrar::processStdOut()
{
    const auto buff = process_.readAllStandardOutput();
    //const auto size = buff.size();
    stdout_.append(buff);
    if (stdout_.isEmpty())
        return;

    std::string temp;
    for (int i=0; i<stdout_.size(); ++i)
    {
        const auto byte = stdout_.at(i); 
        if (byte == '\n')
        {
            const auto line = widen(temp);
            if (line.isEmpty())
                continue;

            DEBUG(line);

            QString str;
            int done;
            if (parseVolume(line, str))
            {
                onExtract(archive_, str);
                files_.insert(str);                
            }
            else if (parseProgress(line, str, done))
            {
                onProgress(archive_, str, done);
            }
            else if (parseMessage(line, str))
            {
                message_ += str;
            }

            if (logFile_.isOpen())
            {
                logFile_.write(temp.data());
                logFile_.write("\n");
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
    const auto buff = process_.readAllStandardError();
    //const auto size = buff.size();
    stderr_.append(buff);
    if (stderr_.isEmpty())
        return;

    std::string temp;
    for (int i=0; i<stderr_.size(); ++i)
    {
        const auto byte = stderr_.at(i);
        if (byte == '\n')
        {
            const auto line = widen(temp);
            if (line.isEmpty())
                continue;

            DEBUG(line);
            if (logFile_.isOpen())
            {
                logFile_.write(temp.data());
                logFile_.write("\n");
            }

            errors_.append(line);

            temp.clear();
        }
        else temp.push_back(byte);
    }
    const auto leftOvers = (int)temp.size();
    stderr_  = stderr_.right(leftOvers);
    success_ = false;
}

void Unrar::processFinished(int exitCode, QProcess::ExitStatus status)
{
    DEBUG("unrar %1 finished. ExitCode %2, ExitStatus %3", 
        unrar_, exitCode, status);

    if (archive_.state != Archive::Status::Stopped)
    {
        if (status == QProcess::CrashExit)
        {
            if (logFile_.isOpen())
            {
                logFile_.write("*** Crash Exit ***");
                logFile_.close();
            }
            archive_.message = toString(status);
            archive_.state   = Archive::Status::Error;

        }
        else if (success_)
        {
            const auto buff = process_.readAllStandardOutput();
            stdout_.append(buff);
            std::string temp(stdout_.constData(),
                stdout_.size());
            message_.append(widen(temp));
            if (logFile_.isOpen())
            {
                logFile_.write(temp.data());
                logFile_.write("\n");
            }

            DEBUG("unrar success, %1", message_);

            archive_.message = message_;
            archive_.state   = Archive::Status::Success;
            if (cleanup_)
            {
                DEBUG("Cleaning up rars %1", archive_.path);

                for (const auto& f : files_)
                {
                    QFile file(archive_.path + "/" + f);
                    file.remove();
                    DEBUG("Removed %1", f);
                }
            }
        }
        else
        {
            const auto buff = process_.readAllStandardError();
            stderr_.append(buff);
            std::string temp(stderr_.constData(),
                stderr_.size());
            errors_.append(widen(temp));
            if (logFile_.isOpen())
            {
                logFile_.write(temp.data());
                logFile_.write("\n");
            }

            DEBUG("unrar failed, errors %1", errors_);
            archive_.message = errors_;
            archive_.state   = Archive::Status::Failed;
        }
    }
    if (logFile_.isOpen())
    {
        logFile_.flush();
        logFile_.close();
    }

    onReady(archive_);
}

void Unrar::processError(QProcess::ProcessError error)
{
    DEBUG("Unrar %1 process error %1", unrar_, error);

    archive_.message = toString(error);
    archive_.state   = Archive::Status::Error;

    if (logFile_.isOpen())
    {
        logFile_.write(stdout_);
        logFile_.write("\n");
        logFile_.write(stderr_);
        logFile_.write("\n");
        logFile_.write("*** Process Error ***");
        logFile_.flush();
        logFile_.close();
    }

    onReady(archive_);
}

void Unrar::processState(QProcess::ProcessState state)
{
    DEBUG("unrar state %1 pid %2", state, process_.pid());
}

} // app
