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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QRegExp>
#  include <QtDebug>
#include "newsflash/warnpop.h"

#include <cstring> // for strlen
#include <string>
#include <map>

#include "unrar.h"
#include "debug.h"
#include "format.h"
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

Unrar::Unrar(const QString& executable)
  : mUnrarExecutable(executable)
{
    mProcess.onFinished = std::bind(&Unrar::onFinished, this);
    mProcess.onStdErr   = std::bind(&Unrar::onStdErr, this,
        std::placeholders::_1);
    mProcess.onStdOut   = std::bind(&Unrar::onStdOut, this,
        std::placeholders::_1);
}


void Unrar::extract(const Archive& arc, const Settings& settings)
{
    mMessage.clear();
    mErrors.clear();
    mCleanupSet.clear();
    mDoCleanup = settings.purgeOnSuccess;

    const QString logFile = settings.writeLog
        ? app::joinPath(arc.path, "extract.log")
        : "";
    const QString& workingDir = arc.path;

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
    mCurrentArchive = arc;

    // start unrarring.
    mProcess.start(mUnrarExecutable, args, logFile, workingDir);

    DEBUG("Started unrar for %1", arc.file);
}

void Unrar::stop()
{
    DEBUG("Killing unrar of archive%1", mCurrentArchive.file);

    mCurrentArchive.state = Archive::Status::Stopped;

    mProcess.kill();
}

bool Unrar::isRunning() const
{
    return mProcess.isRunning();
}

QStringList Unrar::findArchives(const QStringList& fileNames) const
{
    return Unrar::findVolumes(fileNames);
}

bool Unrar::isSupportedFormat(const QString& filePath, const QString& fileName) const
{
    static const char* patterns[] = {
        ".part01.rar",
        ".part001.rar",
        ".r00",
        ".rar"
    };
    for (const char* p : patterns)
    {
        if (fileName.contains(QString::fromLatin1(p)))
            return true;
    }
    return false;
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

QString Unrar::getCopyright(const QString& executable)
{
    QStringList stdout;
    QStringList stderr;

    if (Process::runAndCapture(executable, stdout, stderr))
    {
        for (const auto& line : stdout)
        {
            if (line.toUpper().contains("COPYRIGHT"))
                return line;
        }
    }
    return "";
}

void Unrar::onFinished()
{
    DEBUG("Unrar finished. Archive %1", mCurrentArchive.file);

    // the user killed the whole thing.
    if (mCurrentArchive.state != Archive::Status::Stopped)
    {
        const auto error = mProcess.error();

        if (error == Process::Error::None)
        {
            // we should have extracted at least 1 file
            const auto success = mErrors.isEmpty() && !mCleanupSet.empty();

            if (success)
            {
                INFO("Unrar %1 success.", mCurrentArchive.file);
                mCurrentArchive.message = mMessage;
                mCurrentArchive.state   = Archive::Status::Success;
            }
            else
            {
                WARN("Unrar %1 failed.", mCurrentArchive.file);
                if (mErrors.isEmpty())
                    mCurrentArchive.message = mMessage;
                else mCurrentArchive.message = mErrors;
                if (mCurrentArchive.message.isEmpty())
                {
                    if (mCleanupSet.empty())
                        mCurrentArchive.message = "Unknown failure.";
                }
                mCurrentArchive.state   = Archive::Status::Failed;
            }

            if (success && mDoCleanup)
            {
                DEBUG("Cleaning up rars in %1", mCurrentArchive.path);
                for (const auto& file : mCleanupSet)
                {
                    const auto& name = joinPath(mCurrentArchive.path, file);
                    QFile fileio;
                    fileio.setFileName(name);
                    if (!fileio.remove())
                    {
                        ERROR("Failed to remove (cleanup) %1, %2", name, fileio.error());
                    }
                }
            }
        }
        else if (error == Process::Error::Crashed)
        {
            ERROR("Unrar process crashed when extracting %1", mCurrentArchive.file);
            mCurrentArchive.message = "Crash exit :(";
            mCurrentArchive.state   = Archive::Status::Error;
        }
        else
        {
            ERROR("Unrar process failed to run when extracting %1", mCurrentArchive.file);
            mCurrentArchive.message = "Process error :(";
            mCurrentArchive.state   = Archive::Status::Error;
        }
    }
    onReady(mCurrentArchive);
}

void Unrar::onStdErr(const QString& line)
{
    mErrors += line;
}


void Unrar::onStdOut(const QString& line)
{
    QString str;
    int done = 0;

    if (parseVolume(line, str))
    {
        onExtract(mCurrentArchive, str);
        mCleanupSet.insert(str);
    }
    else if (parseProgress(line, str, done))
    {
        onProgress(mCurrentArchive, str, done);
    }
    else if (parseMessage(line, str))
    {
        mMessage += str;
    }
}

} // app
