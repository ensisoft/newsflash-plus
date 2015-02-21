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

#define LOGTAG "cmds"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QStringList>
#  include <QTextStream>
#  include <QFileInfo>
#  include <QFile>
#  include <QDir>
#  include <QIODevice>
#include <newsflash/warnpop.h>
#include <newsflash/engine/linebuffer.h>
#include "commands.h"
#include "settings.h"
#include "debug.h"
#include "eventlog.h"
#include "engine.h"
#include "distdir.h"

namespace {
    const char* stringify(QProcess::ExitStatus status)
    {
        switch (status)
        {
            case QProcess::NormalExit: return "NormalExit";
            case QProcess::CrashExit:  return "CrashExit";
        }
        Q_ASSERT(0);
        return "";
    }

    const char* stringify(QProcess::ProcessError error)
    {
        switch (error)
        {
            case QProcess::FailedToStart: return "FailedToStart";
            case QProcess::Crashed: return "Crashed";
            case QProcess::Timedout: return "Timedout";
            case QProcess::WriteError: return "WriteError";
            case QProcess::ReadError: return "ReadError";
            case QProcess::UnknownError: return "UnknownError";
        }
        Q_ASSERT(0);
        return "";
    }    
} // namespace

namespace app
{

Commands::Command::Command() : enabled_(true)
{
    static quint32 id = 1;
    guid_ = id++;    
}

Commands::Command::Command(const QString& name, const QString& exec, const QString& file, 
    const QString& args) : Command()
{
    name_ = name;
    exec_ = exec;
    file_ = file;
    args_ = args;
    if (args_.isEmpty())
        args_ = "${target}";
}

QIcon Commands::Command::icon() const
{
    if (!icon_.isNull())
        return icon_;
 
    icon_ = extractIcon(exec_);

    if (icon_.isNull())
        icon_ = QIcon("icons:ico_application.png");

    return icon_;    
}

// QStringList Commands::Command::prepareArgList(const QString& target) const
// {
//     QStringList args = args_.split(" ", QString::SkipEmptyParts);    
//     QStringList ret;
//     ret << file_;
//     for (const auto& arg : args)
//     {
//         if (arg == "${bindir}")
//             ret << distdir::path();
//         else if (arg == "${cwd}")
//             args << QDir::currentPath();
//         else if (arg == "${target}")
//              args << target;
//     }    
//     return ret;
// }

void Commands::Command::startNewInstance(const app::DataFileInfo& info)
{}

void Commands::Command::startNewInstance(const app::FileBatchInfo& info)
{}


Commands::Commands() 
{
    DEBUG("Commands created");
}

Commands::~Commands()
{
    DEBUG("Commands deleted");
}

void Commands::loadState(Settings& settings)
{
    const auto scripts = settings.get("commands", "list").toStringList();
    for (const auto& key : scripts)
    {
        const auto& name = settings.get(key, "name").toString();
        const auto& exec = settings.get(key, "exec").toString();
        const auto& args = settings.get(key, "args").toString();
        const auto& file = settings.get(key, "file").toString();
        const auto onoff = settings.get(key, "enabled", true);

        Command command(name, exec, file, args);
        command.setEnabled(onoff);
        commands_.push_back(command);
        DEBUG(str("Loaded command _1", name));
    }
}

void Commands::saveState(Settings& settings)
{
    for (const auto& command : commands_)
    {
        const auto& key = "command/" + command.name();

    }
}

void Commands::fileCompleted(const app::DataFileInfo& info)
{
    const auto filename = QDir::toNativeSeparators(info.path + "/" + info.name);

    for (const auto& command : commands_)
    {

    }
    //startNextCommand();
}

void Commands::batchCompleted(const app::FileBatchInfo& info)
{
    const auto folder = info.path;

    for (const auto& command : commands_)
    {
        if (!command.isEnabled())
            continue;
        //enqueueCommand(command, folder);
    }
    //startNextCommand();
}

} // app