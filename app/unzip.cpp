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

#define LOGTAG "unzip"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QRegExp>
#  include <QFileInfo>
#  include <QtDebug>
#include "newsflash/warnpop.h"

#include <functional>
#include <string>

#include "debug.h"
#include "unzip.h"
#include "eventlog.h"
#include "utility.h"

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

Unzip::Unzip(const QString& executable) : mUnzipExecutable(executable)
{
    mProcess.onFinished = std::bind(&Unzip::onFinished, this);
    mProcess.onStdErr   = std::bind(&Unzip::onStdErr, this,
        std::placeholders::_1);
    mProcess.onStdOut   = std::bind(&Unzip::onStdOut, this,
        std::placeholders::_1);
}


void Unzip::extract(const Archive& arc, const Settings& settings)
{
    mMessage.clear();
    mErrors.clear();
    mHasProgress = false;

    mDoCleanup = settings.purgeOnSuccess;

    const QString& logFile = settings.writeLog
        ? app::joinPath(arc.path, "extract.log")
        : "";
    const QString& workingDir = arc.path;

    bool fakeTTY = false;

#if defined(LINUX_OS)
    QFileInfo script("/usr/bin/script");
    fakeTTY = script.exists();
#endif

    QString executable;
    QStringList args;

    // see switches
    // https://sevenzip.osdn.jp/chm/cmdline/switches/overwrite.htm

    // 7za is a software that prints different output
    // if it's ran from an interactive shell than if it's ran without a tty session.
    // We'd want it to behave like it does with the interactive shell, i.e.
    // print progress information.
    //
    // Theoretically we should be able to invoke 7za through the script command
    // which would fake the interactive shell session for us.
    // The code below should accomplish that but when the process is started
    // this way then Qt seg faults. ;-(
    //
    // So currently we don't have any progress information available.

    fakeTTY = false;

    if (fakeTTY)
    {
        QString unzip;
        unzip.append("\"");
        unzip.append(" " + mUnzipExecutable + " "); // the 7zip executable
        unzip.append(" e "); // extract
        if (settings.overWriteExisting)
            unzip.append(" -aoa "); // overwrite all existing.
        else unzip.append(" -aou "); // auto rename existing file

        const QString& file = app::joinPath(arc.path, arc.file);
        unzip.append("\\\"" + file + "\\\"");
        unzip.append("\"");

        args << "--return";
        args << "--command";
        args << unzip;
        args << "> /dev/stdout";
        executable = "/usr/bin/script";
        mHasProgress = true;
    }
    else
    {
        args << "e"; // for extract
        if (settings.overWriteExisting)
            args << "-aoa"; // overwrite all existing
        else args << "-aou"; // auto rename existing files

        args << arc.file;
        executable = mUnzipExecutable;
    }

    mHasProgress = fakeTTY;
    mCurrentArchive = arc;

    mProcess.start(executable, args, logFile, workingDir);

    DEBUG("Started unzip for %1", arc.file);
}

void Unzip::stop()
{
    DEBUG("Killing unzip of archive %1", mCurrentArchive.file);

    mCurrentArchive.state = Archive::Status::Stopped;

    mProcess.kill();
}

bool Unzip::isRunning() const
{
    return mProcess.isRunning();
}

bool Unzip::isSupportedFormat(const QString& filePath, const QString& fileName) const
{
    if (fileName.endsWith(".7z") || fileName.endsWith(".zip"))
        return true;
    return false;
}

QStringList Unzip::findArchives(const QStringList& fileNames) const
{
    QStringList ret;

    for (const auto& name : fileNames)
    {
        if (name.endsWith(".7z") || name.endsWith(".zip"))
            ret << name;
    }

    return ret;
}

bool Unzip::hasProgressInfo() const
{
    return mHasProgress;
}

// static
QString Unzip::getCopyright(const QString& executable)
{
    QStringList stdoutLines;
    QStringList stderrLines;

    if (Process::runAndCapture(executable, stdoutLines, stderrLines))
    {
        for (const auto& line : stdoutLines)
        {
            if (line.toUpper().contains("COPYRIGHT"))
                return line;
        }
    }
    return "";
}

// \D matches a non-digit
// \d matches a digit
// \S matches non-whitespace character
// \s matches white space character

// static
bool Unzip::parseVolume(const QString& line, QString& file)
{
    QStringList captures;
    if (isMatch(line, "Extracting archive: (.+)", captures, 1))
    {
        file = captures[0];
        return true;
    }

    return false;
}

// static
bool Unzip::parseProgress(const QString& line, QString& file, int& done)
{
    QStringList captures;
    if (isMatch(line, "(\\d{1,2})% (.*)", captures, 2))
    {
        done = captures[0].toInt();
        file = captures[1];
        return true;
    }
    return false;
}

// static
bool Unzip::parseMessage(const QString& line, QString& msg)
{
    QStringList captures;
    if (isMatch(line, "(Everything is Ok)", captures, 1))
    {
        msg = captures[0];
        return true;
    }
    else if (isMatch(line, "(.*ERROR.*)", captures, 1))
    {
        msg = captures[0];
        return true;
    }
    return false;
}

void Unzip::onFinished()
{
    DEBUG("Unzip finished. Archive %1", mCurrentArchive.file);

    if (mCurrentArchive.state != Archive::Status::Stopped)
    {
        const auto error = mProcess.error();

        if (error == Process::Error::None)
        {
            const auto success = mErrors.isEmpty();
            if (success)
            {
                INFO("Unzip %1 success.", mCurrentArchive.file);
                mCurrentArchive.message = mMessage;
                mCurrentArchive.state   = Archive::Status::Success;
            }
            else
            {
                WARN("Unzip %1 failed", mCurrentArchive.file);
                if (mErrors.isEmpty())
                    mErrors = mMessage;
                if (mErrors.isEmpty())
                    mErrors = "Unknown failure";
                mCurrentArchive.message = mErrors;
                mCurrentArchive.state   = Archive::Status::Failed;
            }

            if (success && mDoCleanup)
            {
                const auto& archive = app::joinPath(mCurrentArchive.path, mCurrentArchive.file);
                DEBUG("Cleaning up archive %1", archive);
                QFile file;
                file.setFileName(archive);
                if (!file.remove())
                {
                    ERROR("Failed to remove (cleanup) %1, %2", archive, file.error());
                }
            }
        }
        else if (error == Process::Error::Crashed)
        {
            ERROR("Unzip process crashed when extracting %1", mCurrentArchive.file);
            mCurrentArchive.message = "Crashed :(";
            mCurrentArchive.state   = Archive::Status::Error;
        }
        else
        {
            ERROR("Unzip process failed to to extract %1", mCurrentArchive.file);
            mCurrentArchive.message = "Process error :(";
            mCurrentArchive.state   = Archive::Status::Error;
        }
    }
    onReady(mCurrentArchive);
}

void Unzip::onStdErr(const QString& line)
{
    mErrors += line;
}

void Unzip::onStdOut(const QString& line)
{
    QString str;
    int done = 0;

    DEBUG("Unzip %1", line);

    if (parseVolume(line, str))
    {
        onExtract(mCurrentArchive, str);
    }
    else if (parseProgress(line, str, done))
    {
        DEBUG("Unzip progress %1 %2", line, done);

        onProgress(mCurrentArchive, str, done);


    }
    else if (parseMessage(line, str))
    {
        mMessage += str;
    }
}




} // namespace
