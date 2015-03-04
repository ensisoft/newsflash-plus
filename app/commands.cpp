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

Commands::Command::Command(QString exec, QString args) : Command()
{
    exec_ = exec;
    args_ = args;
}

Commands::Command::Command(QString exec, QString args, QString comment) : Command(exec, args)
{
    comment_ = comment;
}

Commands::Command::Command(QString exec, QString args, QString comment, Condition cond) : Command(exec, args, comment)
{
    conds_.push_back(std::move(cond));
    enableCond_ = true;
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
    if (!when_.test(When::OnFileDownload))
        return;

    if (enableCond_)
    {
        for (const auto& cond : conds_)
        {
            if (!cond.evaluate(file))
                return;
        }        
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
        else args << item;
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
    if (!when_.test(When::OnPackDownload))
        return;

    if (enableCond_)
    {
        for (const auto& cond : conds_)
        {
            if (!cond.evaluate(pack))
                return;
        }
    }

    QStringList args;
    QStringList list = args_.split(" ", QString::SkipEmptyParts);
    for (const auto& item : list)
    {
        if (item == "${filegroup.path}")
            args << pack.path;
        else args << item;
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
    if (!when_.test(When::OnArchiveUnpack))
        return;

    if (enableCond_)
    {
        for (const auto& cond : conds_)
        {
            if (!cond.evaluate(arc))
                return;
        }
    }

    QStringList args;
    QStringList list = args_.split(" ", QString::SkipEmptyParts);
    for (const auto& item : list)
    {
        if (item == "${archive.path}")
            args << arc.path;
        else if (item == "${archive.file}")
            args << arc.file;
        else args << item;
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
    if (!when_.test(When::OnArchiveRepair))
        return;

    if (enableCond_)
    {
        for (const auto& cond : conds_)
        {
            if (!cond.evaluate(arc))
                return;
        }
    }

    QStringList args;
    QStringList list = args_.split(" ", QString::SkipEmptyParts);
    for (const auto& item : list)
    {
        if (item == "${archive.path}")
            args << arc.path;
        else if (item == "${archive.file}")
            args << arc.file;
        else args << item;
    }
    if (!QProcess::startDetached(exec_, args))
    {
        ERROR("Failed to execute command %1", exec_);
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

        std::vector<Condition> cv;

        const auto conds = settings.get(key, "conditions").toStringList();
        for (const auto& c : conds)
        {
            const auto toks = c.split("|");
            if (toks.size() != 3)
                throw std::runtime_error("command condition parse error");

            cv.emplace_back(toks[0], toks[1], toks[2]);
        }

        Command command(exec, args);
        command.setEnableCommand(cmd);
        command.setEnableCondition(cond);
        command.setWhen(flags);
        command.setComment(com);
        command.setConditionsCopy(std::move(cv));
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

        QStringList conds;
        const auto& conditions = command.getConditions();
        for (const auto& condition : conditions)
        {
            QString str;
            str.append(condition.getLHS());
            str.append("|");
            str.append(condition.getOp());
            str.append("|");
            str.append(condition.getRHS());
            conds << str;
        }
        settings.set(key, "conditions", conds);

        DEBUG("Saved command %1", command.exec());
        commands << key;
    }
    settings.set("commands", "list", commands);
}

void Commands::firstLaunch()
{
#if defined(LINUX_OS)
    Command vlc("/usr/bin/vlc", "${file.file}", "Play video files in a video player.",
        Condition("file.type", "equals", toString(FileType::Video)));
    vlc.setWhen(Command::When::OnFileDownload);

    Command eog("/usr/bin/eog", "${file.file}", "Open images in an image viewer.",
        Condition("file.type", "equals", toString(FileType::Image)));
    eog.setWhen(Command::When::OnFileDownload);

    Command audacious("/usr/bin/audacious", "--enqueue ${file.file}", "Enqueue music files in an audio player.",
        Condition("file.type", "equals", toString(FileType::Audio)));
    audacious.setWhen(Command::When::OnFileDownload);

    commands_.push_back(vlc);
    commands_.push_back(eog);
    commands_.push_back(audacious);

#elif defined(WINDOWS_OS)

#endif    
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