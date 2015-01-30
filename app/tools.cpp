// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#define LOGTAG "tools"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QProcess>
#  include <QFileInfo>
#include <newsflash/warnpop.h>

#include "settings.h"
#include "platform.h"
#include "format.h"
#include "debug.h"
#include "tools.h"
#include "eventlog.h"

namespace app
{

tools* g_tools;

tools::tool::tool() : types_(0)
{
    // generate a unique id
    static quint32 guid = 1;
    guid_ = ++guid;

    arguments_ = "${file}";
}

tools::tool::~tool()
{}

void tools::tool::startNewInstance(const QString& file) const 
{
    QString args = arguments_;
    args.replace("${file}", QString("\"%1\"").arg(file));

    // we have to call the single argument version of startDetached
    // because its unknown how many extra arguments have been configured for the
    // tool (without starting to parse it...)
    // and Qt encloses each QStringList item in "" marks which makes them 
    // single arguments to the child process.
    if (!QProcess::startDetached(QString("\"%1\" %2").arg(binary_).arg(args)))
    {
        ERROR(str("Failed to execute command _1 _2", binary_, args));
        return;
    }
}

const QIcon& tools::tool::icon() const 
{
    if (!icon_.isNull())
        return icon_;

    // try to extract the icon from the binary.
    // if that fails then we choose the icon of the first 
    // tagged file type.

    icon_ = extract_icon(binary_);
    if (icon_.isNull())
    {
        auto beg = filetype_iterator::begin();
        auto end = filetype_iterator::end();
        for (; beg != end; ++beg)
        {
            if (types_.test(*beg))
            {
                icon_ = find_fileicon(*beg);
                break;
            }
        }
    }
    return icon_;
}

void tools::loadstate(app::settings& store)
{
    QStringList list = store.get("tools", "list").toStringList();
    for (const auto& key : list)
    {
        const auto& name = store.get(key, "name").toString();
        const auto& binary = store.get(key, "binary").toString();
        const auto& arguments = store.get(key, "arguments").toString();
        const auto& shortcut = store.get(key, "shortcut").toString();
        const auto& types = store.get(key, "types").toLongLong();

        tools::tool tool;
        tool.setName(name);
        tool.setBinary(binary);
        tool.setArguments(arguments);
        tool.setShortcut(shortcut);
        tool.setTypes(types);
        tools_.push_back(std::move(tool));

        DEBUG(str("Loaded tool _1", name));
    }
}

void tools::savestate(app::settings& store)
{
    QStringList list;
    for (const auto& tool : tools_)
    {
        QString key = tool.name();
        store.set(key, "name", tool.name());
        store.set(key, "binary", tool.binary());
        store.set(key, "arguments", tool.arguments());
        store.set(key, "shortcut", tool.shortcut());
        store.set(key, "types", tool.types().value());
        list << key;
    }
    store.set("tools", "list", list);    
}

std::vector<const tools::tool*> tools::get_tools() const 
{
    std::vector<const tool*> ret;
    for (const auto& t : tools_)
        ret.push_back(&t);

    return ret;
}

std::vector<const tools::tool*> tools::get_tools(bitflag types) const 
{
    std::vector<const tool*> ret;
    for (const auto& t : tools_)
    {
        const auto& flags = t.types();
        if (flags.test(types))
            ret.push_back(&t);
    }
    return ret;
}

void tools::set_tools_copy(std::vector<tool> new_tools)
{
    tools_ = std::move(new_tools);

    emit toolsUpdated();
}

tools::tool* tools::get_tool(quint32 guid)
{
    auto it = std::find_if(std::begin(tools_), std::end(tools_), 
        [=](const tool& maybe) {
            return maybe.guid() == guid;
        });
    if (it == std::end(tools_))
        return nullptr;

    return &(*it);
}

void tools::add_tool(tools::tool tool)
{
    tools_.push_back(std::move(tool));

    emit toolsUpdated();
}

void tools::del_tool(std::size_t i)
{
    auto it = tools_.begin();
    std::advance(it, i);
    tools_.erase(it);

    emit toolsUpdated();
}

bool tools::has_tool(const QString& executable)
{
    const QString base = QFileInfo(executable).completeBaseName();

    const auto it = std::find_if(std::begin(tools_), std::end(tools_),
        [&](const tool& t) {
            const QString b = QFileInfo(t.binary()).completeBaseName();
            return b == base;
        });

    return it != std::end(tools_);
}


} // app