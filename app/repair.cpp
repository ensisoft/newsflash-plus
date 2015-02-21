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

#define LOGTAG "repair"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#include <newsflash/warnpop.h>
#include <newsflash/engine/linebuffer.h>
#include "repair.h"
#include "debug.h"
#include "eventlog.h"
#include "engine.h"
#include "distdir.h"

namespace app
{

RepairEngine::RepairEngine()
{
    DEBUG("RepairEngine created");

    QObject::connect(&process_, SIGNAL(finished(int, QProcess::ExitStatus)), 
        this, SLOT(processFinished(int, QProcess::ExitStatus)));
    QObject::connect(&process_, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(processError(QProcess::ProcessError)));
    QObject::connect(&process_, SIGNAL(readyReadStandardOutput()),
        this, SLOT(processStdOut()));
    QObject::connect(&process_, SIGNAL(readyReadStandardError()),
        this, SLOT(processStdErr()));        

    QObject::connect(g_engine, SIGNAL(fileCompleted(const app::DataFileInfo&)),
        this, SLOT(fileComplete(const app::DataFileInfo&)));
    QObject::connect(g_engine, SIGNAL(batchCompleted(const app::FileBatchInfo&)),
        this, SLOT(batchComplete(const app::FileBatchInfo&)));    
}

RepairEngine::~RepairEngine()
{
    DEBUG("RepairEngine deleted");
}

QVariant RepairEngine::data(const QModelIndex& index, int role) const
{
    return {};
}

QVariant RepairEngine::headerData(int section, Qt::Orientation orientation, int role) const
{
    return {};
}

int RepairEngine::rowCount(const QModelIndex&) const 
{
    return 0;
}

int RepairEngine::columnCount(const QModelIndex&) const 
{
    return 0;
}

void RepairEngine::sort(int column, Qt::SortOrder order)
{}

void RepairEngine::processStdOut()
{
    const auto buff = process_.readAllStandardOutput();
    const auto size = buff.size();
    //DEBUG(str("Command '_1' stdout _2 bytes", name_, size));

    stdout_.append(buff);

    const nntp::linebuffer lines(stdout_.constData(), stdout_.size());
    auto beg = lines.begin();
    auto end = lines.end();
    for (; beg != end; ++beg)
    {
        auto line = beg.to_str();
        if (line.back() == '\n')
            line.pop_back();
        if (line.back() == '\r')
            line.pop_back();
    
        //emit writeStdOut(widen(line));
    }
    stdout_.chop(beg.pos());
}

void RepairEngine::processStdErr()
{
    const auto buff = process_.readAllStandardError();
    const auto size = buff.size();
    //DEBUG(str("Command '_1' stderr _2 bytes", name_, size));

    stderr_.append(buff);

    const nntp::linebuffer lines(stderr_.constData(), stderr_.size());
    auto beg = lines.begin();
    auto end = lines.end();
    for (; beg != end; ++beg)
    {
        auto line = beg.to_str();
        if (line.back() == '\n')
            line.pop_back();
        if (line.back() == '\r')
            line.pop_back();

        //emit writeStdErr(widen(line));
    }
    stderr_.chop(beg.pos());
}

void RepairEngine::processFinished(int exitCode, QProcess::ExitStatus status)
{
    //DEBUG(str("Command '_1' finished. ExitCode _2, ExitStatus _3",
    //    name_, exitCode, stringify(status)));
}

void RepairEngine::processError(QProcess::ProcessError error)
{
    //DEBUG(str("Command '_1' error. _2", name_, stringify(error)));
}


void RepairEngine::fileCompleted(const app::DataFileInfo& info)
{

}

void RepairEngine::batchCompleted(const app::FileBatchInfo& info)
{}

RepairEngine* g_repair;


} // app
