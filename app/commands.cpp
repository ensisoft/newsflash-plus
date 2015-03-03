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
#  include <QProcess>
#include <newsflash/warnpop.h>
#include <newsflash/engine/linebuffer.h>
#include "commands.h"
#include "settings.h"
#include "debug.h"
#include "eventlog.h"
#include "engine.h"
#include "distdir.h"
#include "fileinfo.h"
#include "archive.h"

namespace app
{

bool evaluateOp(const QString& lhs, const QString& op, const QString& rhs)
{
    if (op == "equals")
        return lhs == rhs;
    else if (op == "not equals")
        return lhs != rhs;
    else if (op == "contains")
        return lhs.contains(rhs) != -1;
    else if (op == "not contains")
        return lhs.contains(rhs) == -1;

    return false;
}

bool Commands::Condition::evaluate(const app::FileInfo& info) const 
{
    QString lhs;
    if (lhs_ == "file.type")
        lhs = toString(info.type);
    else if (lhs_ == "file.file")
        lhs = QDir::toNativeSeparators(info.path + "/" + info.name);
    else if (lhs_ == "file.name")
        lhs = info.name;
    else if (lhs_ == "file.path")
        lhs = info.path;

    return evaluateOp(lhs, op_, rhs_);
}

bool Commands::Condition::evaluate(const app::FilePackInfo& info) const 
{
    QString lhs;
    if (lhs_ == "filegroup.path")
        lhs = info.path;

    return evaluateOp(lhs, op_, rhs_);
}

bool Commands::Condition::evaluate(const app::Archive& arc) const 
{
    QString lhs;
    if (lhs_ == "archive.path")
        lhs = arc.path;
    else if (lhs_ == "archive.file")
        lhs = arc.file;
    else if (lhs_ == "archive.success")
        lhs = toString(arc.state == Archive::Status::Success);

    return evaluateOp(lhs, op_, rhs_);
}

Commands::Command::Command() : enableCommand_(true), enableCond_(false)
{
    static quint32 id = 1;
    guid_ = id++;    
    args_ = "${file.file}";
}

Commands::Command::Command(const QString& exec, const QString& args) : Command()
{
    exec_ = exec;
    args_ = args;
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

void Commands::Command::onFile(const app::FileInfo& file)
{
    if (!enableCommand_)
        return;

    if (enableCond_)
    {
        if (!cond_.evaluate(file))
            return;
    }

    QStringList args;
    QStringList list = args_.split(" ", QString::SkipEmptyParts);
    for (const auto& item : list)
    {
        if (item == "${file.name}")
            args << file.name;
        else if (item == "${file.path}")
            args << file.path;
        else if (item == "${file.file}")
            args << QDir::toNativeSeparators(file.path + "/" + file.name);
    }
    if (!QProcess::startDetached(exec_, args))
    {
        ERROR("Failed to execute command %1", exec_);
    }
}

void Commands::Command::onFilePack(const app::FilePackInfo& pack)
{
    if (!enableCommand_)
        return;

    if (enableCond_)
    {
        if (!cond_.evaluate(pack))
            return;
    }

    QStringList args;
    QStringList list = args_.split(" ", QString::SkipEmptyParts);
    for (const auto& item : list)
    {
        if (item == "${filegroup.path}")
            args << pack.path;
    }
    if (!QProcess::startDetached(exec_, args))
    {
        ERROR("Failed to execute command %1", exec_);
    }
}

void Commands::Command::onUnpack(const app::Archive& arc)
{
    if (!enableCommand_)
        return;

    if (enableCond_)
    {
        if (!cond_.evaluate(arc))
            return;
    }

    QStringList args;
    QStringList list = args_.split(" ", QString::SkipEmptyParts);
    for (const auto& item : list)
    {
        if (item == "${archive.path}")
            args << arc.path;
        else if (item == "${archive.file}")
            args << arc.file;
    }
    if (!QProcess::startDetached(exec_, args))
    {
        ERROR("Failed to execute command %1", exec_);
    }

}

void Commands::Command::onRepair(const app::Archive& arc)
{
    if (!enableCommand_)
        return;

    if (enableCond_)
    {
        if (!cond_.evaluate(arc))
            return;
    }
}


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
    Command::WhenFlags defaultFlags;

    const auto scripts = settings.get("commands", "list").toStringList();
    for (const auto& key : scripts)
    {
        const auto exec  = settings.get(key, "exec").toString();
        const auto args  = settings.get(key, "args").toString();
        const auto com   = settings.get(key, "comment").toString();
        const auto cmd   = settings.get(key, "enable_command", true);
        const auto cond  = settings.get(key, "enable_condition", false);
        const auto flags = settings.get(key, "flags", defaultFlags.value());
        Command::WhenFlags when;
        when.set_from_value(flags);

        Command command(exec, args);
        command.setEnableCommand(cmd);
        command.setEnableCondition(cond);
        command.setWhen(when);
        command.setComment(com);

        commands_.push_back(command);
        DEBUG("Loaded command %1", exec);
    }
}

void Commands::saveState(Settings& settings)
{
    QStringList commands;

    for (const auto& command : commands_)
    {
        const auto& key = "command/cmd" + QString::number(command.guid());
        settings.set(key, "exec", command.exec());
        settings.set(key, "args", command.args());
        settings.set(key, "comment", command.comment());
        settings.set(key, "enable_command", command.isEnabled());
        settings.set(key, "enable_condition", command.isCondEnabled());
        settings.set(key, "flags", command.when().value());

        DEBUG("Saved command %1", command.exec());
        commands << key;
    }
    settings.set("commands", "list", commands);
}

void Commands::fileCompleted(const app::FileInfo& info)
{
    for (auto& command : commands_)
    {
        command.onFile(info);
    }
}

void Commands::packCompleted(const app::FilePackInfo& pack)
{
    for (auto& command : commands_)
    {
        command.onFilePack(pack);
    }
}

void Commands::repairFinished(const app::Archive& arc)
{
    for (auto& command : commands_)
    {
        command.onRepair(arc);
    }
}

void Commands::unpackFinished(const app::Archive& arc)
{
    for (auto& command : commands_)
    {
        command.onUnpack(arc);
    }
}

} // app