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
#include "distdir.h"

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

Unrar::Unrar()
{
    QObject::connect(&process_, SIGNAL(finished(int, QProcess::ExitStatus)), 
        this, SLOT(processFinished(int, QProcess::ExitStatus)));
    QObject::connect(&process_, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(processError(QProcess::ProcessError)));
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
        "We should either wait for it's completion or stop it");        
}

void Unrar::extract(const Archive& arc)
{
    const auto state = process_.state();
    Q_ASSERT(state == QProcess::NotRunning &&
        "We already have a current archive being extracted");

    stdout_.clear();
    stderr_.clear();

    const auto& unrar = distdir::path("unrar");
    const auto& file  = arc.path + "/" + arc.file;

    QStringList args;
    args << "e"; // for extract
    args << arc.file;
    process_.setWorkingDirectory(arc.path);
    process_.setProcessChannelMode(QProcess::MergedChannels);
    DEBUG("Starting unrar for %1", arc.desc);

    archive_ = arc;
}

void Unrar::stop()
{
    const auto state = process_.state();
    if (state != QProcess::NotRunning)
        return;

    // terminate will ask the process nicely to exit
    // QProcess::kill will just kill it forcefully.
    process_.terminate();
}

void Unrar::configure(const Settings& settings)
{
    // todo:
}

bool Unrar::parseVolume(const QString& line, QString& volume)
{
    QStringList captures;
    if (isMatch(line, "Extracting from (\\S+)", captures, 1))
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

void Unrar::processStdOut()
{
    const auto buff = process_.readAllStandardOutput();
    const auto size = buff.size();
    DEBUG("Unrar read %1 bytes", size);

    stdout_.append(buff);

    std::string line;

    int chop;
    for (chop=0; chop<stdout_.size(); ++chop)
    {
        const auto byte = stdout_.at(chop); 
        if (byte == '\n')
        {
            const auto wide = widen(line);
            if (wide.isEmpty())
                continue;

            updateState(wide);

            line.clear();
        }
        else if (byte == '\b')
        {
            line.push_back('.');
        }
        else line.push_back(byte);
    }
    stdout_.chop(chop - line.size());
}

void Unrar::processStdErr()
{
    Q_ASSERT(!"We should be using merged IO channels for stdout and stderr."
        "See the code in startNextRecovery()");
}


void Unrar::processFinished(int exitCode, QProcess::ExitStatus status)
{
}

void Unrar::processError(QProcess::ProcessError error)
{
    DEBUG("Unrar error %1", error);



    // auto& active = list_->getArchive(index);

    // active.state   = Archive::Status::Error;
    // active.message = "Process error";
    // ERROR(str("Unpack '_1' error _2", active.desc, active.message));

    // list_->refresh(index);

    // unpackReady(active);

    // startNextUnpack();
}


void Unrar::updateState(const QString& line)
{
    QString name;
    if (parseVolume(line, name))
    {
        Archiver::Volume vol;
        vol.file = name;
        emit listVolume(archive_, vol);
        return;
    }

    int done;
    if (parseProgress(line, name, done))
    {
        Archiver::File file;
        file.name = name;
        emit progress(archive_, file, done);
    }
}

} // app