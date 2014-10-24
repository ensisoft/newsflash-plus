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

#define LOGTAG "engine"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QCoreApplication>
#  include <QEvent>
#  include <QFile>
#  include <QDir>
#  include <QBuffer>
#include <newsflash/warnpop.h>
#include <newsflash/engine/account.h>
#include <newsflash/engine/nntp.h>

#if defined(WINDOWS_OS)
#  include <windows.h>
#elif defined(LINUX_OS)
#  include <sys/vfs.h>
#endif

#include "engine.h"
#include "debug.h"
#include "homedir.h"
#include "format.h"
#include "eventlog.h"
#include "account.h"
#include "settings.h"
#include "nzbparse.h"

namespace nf = newsflash;
namespace ui = newsflash::ui;

namespace {
    // this event object is posted to the application's
    // event queue when the core engine has pending actions.
    // the callback from the engine comes from anothread thread
    // and we post the event to the app's main thread's event
    // queue which then picks up the event and calls the engine's 
    // message dispatcher.
    class AsyncNotifyEvent : public QEvent
    {
    public:
        AsyncNotifyEvent() : QEvent(identity())
        {}
       ~AsyncNotifyEvent()
        {}

        static QEvent::Type identity() 
        {
            static auto id = QEvent::registerEventType();
            return (QEvent::Type)id;
        }
    private:
    };
} // namespace

namespace app
{

engine::engine()
{

    diskspace_ = 0;

    engine_.on_error = std::bind(&engine::on_engine_error, this,
        std::placeholders::_1);
    engine_.on_file  = std::bind(&engine::on_engine_file, this,
        std::placeholders::_1);

    engine_.on_async_notify = [=]() {
        DEBUG("on_async_notify callback");
        QCoreApplication::postEvent(this, new AsyncNotifyEvent);
    };

    installEventFilter(this);

    DEBUG("Engine created");
}

engine::~engine()
{
    DEBUG("Engine destroyed");
}

void engine::set(const account& acc)
{
    newsflash::account a;
    a.id                    = acc.id;
    a.name                  = to_utf8(acc.name);
    a.username              = to_utf8(acc.username);
    a.password              = to_utf8(acc.password);
    a.secure_host           = latin(acc.secure_host);
    a.secure_port           = acc.secure_port;    
    a.general_host          = latin(acc.general_host);
    a.general_port          = acc.general_port;    
    a.enable_compression    = acc.enable_compression;
    a.enable_pipelining     = acc.enable_pipelining;
    a.enable_general_server = acc.enable_general_server;
    a.enable_secure_server  = acc.enable_secure_server;
    a.connections           = acc.max_connections;
    engine_.set(a);
}

void engine::del(const account& acc)
{
    // todo:
    //engine_.del(const newsflash::account &acc)
}

void engine::set_fill_account(quint32 id)
{
    //engine_.set_fill_account(id);
}

void engine::download_nzb_contents(quint32 acc, const QString& path, const QString& desc, const QByteArray& buff)
{
    QBuffer io(const_cast<QByteArray*>(&buff));

    std::vector<nzbcontent> items;

    const auto err = parse_nzb(io, items);
    switch (err)
    {
        case nzberror::none: break;

        case nzberror::xml:
            ERROR(str("NZB XML parse error (_1)", desc));
            return;

        case nzberror::nzb:
            ERROR(str("NZB content error (_1)", desc));
            return;

        default:
            ERROR(str("Error reading NZB (_1)", desc));
            return;
    }

    QString location = path.isEmpty() ? 
        downloads_ : path;
    location.append("/");
    location.append(desc);

    QDir dir;
    if (!dir.mkpath(location))
    {
        ERROR(str("Error creating path _1", dir));
        return;
    }

    newsflash::ui::download download;
    download.account = acc;
    download.path = narrow(location);
    download.desc = to_utf8(desc);
    for (auto& item : items)
    {
        newsflash::ui::download::file file;
        file.articles = std::move(item.segments);
        file.groups   = std::move(item.groups);
        file.size     = item.bytes;
        file.name     = nntp::find_filename(to_utf8(item.subject));
        if (file.name.size() < 5)
            file.name = to_utf8(item.subject);
        download.files.push_back(std::move(file));
    }
    engine_.download(std::move(download));
    if (connect_)
    {
        QDir dir;
        if (!dir.mkpath(logifiles_))
        {
            ERROR(str("Error creating log path _1", dir));
        }
        engine_.start(to_utf8(logifiles_));
    }


    INFO(str("Downloading _1", desc));
}

void engine::loadstate(settings& s)
{
    logifiles_ = s.get("engine", "logfiles", 
        QDir::toNativeSeparators(QDir::tempPath() + "/Newsflash"));
    downloads_ = s.get("engine", "downloads", 
        QDir::toNativeSeparators(QDir::homePath() + "/Downloads"));

    const auto overwrite = s.get("engine", "overwrite_existing_files", false);
    const auto discard   = s.get("engine", "discard_text", true);
    const auto secure    = s.get("engine", "prefer_secure", true);
    const auto throttle  = s.get("engine", "throttle", false);
    const auto throttleval = s.get("engine", "throttle_value", 0);
    connect_ = s.get("engine", "connect", true);

    engine_.set_overwrite_existing_files(overwrite);
    engine_.set_discard_text_content(discard);
    engine_.set_throttle(throttle);
    engine_.set_throttle_value(throttleval);
}

bool engine::savestate(settings& s)
{
    const auto overwrite = engine_.get_overwrite_existing_files();
    const auto discard   = engine_.get_discard_text_content();
    const auto secure    = engine_.get_prefer_secure();
    //const auto throttle  = engine_.get_th
    const auto throttle = false;

    s.set("engine", "throttle", throttle);
    s.set("engine", "overwrite_existing_files", overwrite);
    s.set("engine", "discard_text", discard);
    s.set("engine", "prefer_secure", secure);
    s.set("engine", "logfiles", logifiles_);
    s.set("engine", "downloads", downloads_);
    s.set("engine", "connect", connect_);
    return true;
}

void engine::connect(bool on_off)
{ 
    connect_ = on_off;
    if (!connect_)
        engine_.stop();
    else
    {
        QDir dir;
        if (!dir.mkpath(logifiles_))
        {
            ERROR(str("Error creating log path _1", dir));
        }
        engine_.start(to_utf8(logifiles_));
    }
}

void engine::refresh()
{
    diskspace_ = 0;

    // rescan the default download location volume
    // for free space. note that the location might 
    // point to a folder that doesn't yet exist.. 
    // so traverse the path towards the root untill 
    // an existing path is found.
    QDir dir(downloads_);
    auto path = dir.absolutePath();
    auto list = path.split("/");

    while (!list.isEmpty())
    {
        path = list.join("/");
        if (QDir(path).exists())
            break;
        list.removeLast();
    }

    const auto native = QDir::toNativeSeparators(path);    

#if defined(WINDOWS_OS)

    ULARGE_INTEGER free = {};
    if (!GetDiskFreeSpaceEx(native.utf16(), &free, nullptr, nullptr))
        return;

    diskspace_ = free.QuadPart;

#elif defined(LINUX_OS)

    const auto u8 = narrow(native);

    struct statfs64 st = {};
    if (statfs64(u8.c_str(), &st) == -1)
        return;

    // f_bsize is the "optimal transfer block size"
    // not sure if this reliable and always the same as the
    // actual file system block size. If it is different then
    // this is incorrect.  
    diskspace_ = st.f_bsize * st.f_bavail;

#endif
}

bool engine::eventFilter(QObject* object, QEvent* event)
{
    if (object == this && event->type() == AsyncNotifyEvent::identity())
    {
        //DEBUG("got it!");
        engine_.pump();
        return true;
    }
    return QObject::eventFilter(object, event);
}

void engine::on_engine_error(const newsflash::ui::error& e)
{}

void engine::on_engine_file(const newsflash::ui::file& f)
{}

engine* g_engine;


} // app
