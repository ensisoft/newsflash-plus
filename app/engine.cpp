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
    diskspace_  = 0;
    totalspeed_ = 0;
    shutdown_  = false;


    engine_.reset(new newsflash::engine);
    engine_->set_error_callback(std::bind(&engine::on_error, this,
        std::placeholders::_1));
    engine_->set_file_callback(std::bind(&engine::on_file_complete, this,
         std::placeholders::_1));
    engine_->set_batch_callback(std::bind(&engine::on_batch_complete, this,
        std::placeholders::_1));
    engine_->set_list_callback(std::bind(&engine::on_list_complete, this,
        std::placeholders::_1));

    // remember that the notify callback can come from any thread
    // within the engine and it has to be thread safe.
    // so we simply post a notification to the main threads
    // event queue and then handle it in the eventFilter
    engine_->set_notify_callback([=]() {
        QCoreApplication::postEvent(this, new AsyncNotifyEvent);
    });

    installEventFilter(this);
    ticktimer_ = startTimer(1000);

    DEBUG("Engine created");
}

engine::~engine()
{


    engine_.reset();

    DEBUG("Engine destroyed");
}

void engine::set(const account& acc)
{
    newsflash::account a;
    a.id                    = acc.id;
    a.name                  = to_utf8(acc.name);
    a.username              = to_utf8(acc.username);
    a.password              = to_utf8(acc.password);
    a.secure_host           = to_latin(acc.secure_host);
    a.secure_port           = acc.secure_port;    
    a.general_host          = to_latin(acc.general_host);
    a.general_port          = acc.general_port;    
    a.enable_compression    = acc.enable_compression;
    a.enable_pipelining     = acc.enable_pipelining;
    a.enable_general_server = acc.enable_general_server;
    a.enable_secure_server  = acc.enable_secure_server;
    a.connections           = acc.max_connections;

    engine_->set_account(a);
}

void engine::del(const account& acc)
{
    // todo:
    //engine_.del(const newsflash::account &acc)
}

void engine::set_fill_account(quint32 id)
{
    engine_->set_fill_account(id);
}

bool engine::download_nzb_contents(quint32 acc, const QString& path, const QString& desc, const QByteArray& buff)
{
    QBuffer io(const_cast<QByteArray*>(&buff));

    std::vector<nzbcontent> items;

    const auto err = parse_nzb(io, items);
    switch (err)
    {
        case nzberror::none: break;

        case nzberror::xml:
            ERROR(str("NZB XML parse error (_1)", desc));
            break;

        case nzberror::nzb:
            ERROR(str("NZB content error (_1)", desc));
            break;

        default:
            ERROR(str("Error reading NZB (_1)", desc));
            break;
    }
    if (err != nzberror::none)
        return false;

    QString location = path.isEmpty() ? 
        downloads_ : path;
    location.append("/");
    location.append(desc);

    QDir dir(location);
    if (!dir.mkpath(location))
    {
        ERROR(str("Error creating path _1", dir));
        return false;
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
    engine_->download(std::move(download));
    if (connect_)
    {
        QDir dir;
        if (!dir.mkpath(logifiles_))
        {
            ERROR(str("Error creating log path _1", dir));
        }
        engine_->start(to_utf8(logifiles_));
    }
    INFO(str("Downloading \"_1\"", desc));
    NOTE(str("Downloading \"_1\"", desc));

    emit newDownloadQueued(desc);

    return true;
}


bool engine::download_nzb_contents(quint32 acc, const QString& path, const QString& desc, 
    const std::vector<const nzbcontent*>& nzb)
{
    QString location = path.isEmpty() ?
        downloads_ : path;
    location.append("/");
    location.append(desc);

    QDir dir(location);
    if (!dir.mkpath(location))
    {
        ERROR(str("Error creating path _1", dir));
        return false;
    }

    newsflash::ui::download download;
    download.account = acc;
    download.path    = narrow(location);
    download.desc    = to_utf8(desc);
    for (auto* item : nzb)
    {
        newsflash::ui::download::file file;
        file.articles = item->segments;
        file.groups   = item->groups;
        file.size     = item->bytes;
        file.name     = nntp::find_filename(to_utf8(item->subject));
        if (file.name.size() < 5)
            file.name = to_utf8(item->subject);
        download.files.push_back(std::move(file));
    }
    engine_->download(std::move(download));
    if (connect_)
    {
        QDir dir;
        if (!dir.mkpath(logifiles_))
        {
            ERROR(str("Error creating log path _1", dir));
        }
        engine_->start(to_utf8(logifiles_));
    }
    INFO(str("Downloading \"_1\"", desc));
    NOTE(str("Downloading \"_1\"", desc));

    emit newDownloadQueued(desc);
    return true;
}

void engine::retrieve_newsgroup_listing(quint32 acc)
{
    engine_->list_newsgroups(acc);
}


void engine::loadstate(settings& s)
{
    logifiles_ = s.get("engine", "logfiles", 
        QDir::toNativeSeparators(QDir::tempPath() + "/Newsflash"));
    downloads_ = s.get("engine", "downloads", 
        QDir::toNativeSeparators(QDir::homePath() + "/Downloads"));

    mountpoint_ = resolveMountPoint(downloads_);

    const auto overwrite = s.get("engine", "overwrite_existing_files", false);
    const auto discard   = s.get("engine", "discard_text", true);
    const auto secure    = s.get("engine", "prefer_secure", true);
    const auto throttle  = s.get("engine", "throttle", false);
    const auto throttleval = s.get("engine", "throttle_value", 0);
    connect_ = s.get("engine", "connect", true);

    engine_->set_overwrite_existing_files(overwrite);
    engine_->set_discard_text_content(discard);
    engine_->set_throttle(throttle);
    engine_->set_throttle_value(throttleval);
    engine_->set_prefer_secure(secure);
}

bool engine::savestate(settings& s)
{
    const auto overwrite = engine_->get_overwrite_existing_files();
    const auto discard   = engine_->get_discard_text_content();
    const auto secure    = engine_->get_prefer_secure();
    const auto throttle  = engine_->get_throttle();
    const auto throttleval = engine_->get_throttle_value();

    s.set("engine", "throttle", throttle);
    s.set("engine", "throttle_value", throttleval);
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
        engine_->stop();
    else
    {
        QDir dir;
        if (!dir.mkpath(logifiles_))
        {
            ERROR(str("Error creating log path _1", dir));
        }
        engine_->start(to_utf8(logifiles_));
    }
}

void engine::refresh()
{
    // rescan the default download location volume
    // for free space. note that the location might 
    // point to a folder that doesn't yet exist.. 
    // so traverse the path towards the root untill 
    // an existing path is found.
    diskspace_ = app::getFreeDiskSpace(mountpoint_);
}

bool engine::shutdown()
{
    killTimer(ticktimer_);    
    shutdown_  = true;
    ticktimer_ = 0;

    engine_->stop();

    if (!engine_->pump())
        return true;

    DEBUG("Engine has pending actions");
    return false;
}

bool engine::eventFilter(QObject* object, QEvent* event)
{
    if (object == this && event->type() == AsyncNotifyEvent::identity())
    {
        if (!engine_->pump())
        {
            if (shutdown_)
            {
                DEBUG("Engine shutdown complete");
                emit shutdownComplete();
            }
        }
        return true;
    }
    return QObject::eventFilter(object, event);
}

void engine::timerEvent(QTimerEvent* event)
{
    // service the engine periodically
    engine_->tick();
}

void engine::on_error(const newsflash::ui::error& e)
{
    const auto resource = from_utf8(e.resource);
    const auto code = e.code;

    if (code)
    {
        std::stringstream ss;
        ss << code;

        ERROR(str("_1 _2", resource, ss.str()));
    }
    else
    {
        ERROR(str("_1 _2", resource, e.what));
    }
}

void engine::on_file_complete(const newsflash::ui::file& f)
{
    QString path = widen(f.path);
    if (path.isEmpty())
        path = QDir::currentPath();

    path = QDir(path).absolutePath();
    path = QDir::toNativeSeparators(path);

    app::file file;
    file.binary  = f.binary;
    file.damaged = f.damaged;
    file.name    = widen(f.name);
    file.path    = path;
    file.size    = f.size;

    DEBUG(str("Downloaded \"_1/_2\"", path, file.name));

    if (f.damaged)
    {
        WARN(str("Completed \"_1\"", file.name));
    }
    else
    {
        INFO(str("Completed \"_1\"", file.name));
    }

    emit fileCompleted(file);

    NOTE(str("\"_1\" is ready", file.name));
}

void engine::on_batch_complete(const newsflash::ui::batch& b)
{
    QString path = widen(b.path);

    DEBUG(str("Batch complete \"_1\"", path));

    path = QDir(path).absolutePath();
    path = QDir::toNativeSeparators(path);


}

void engine::on_list_complete(const newsflash::ui::listing& l)
{
    DEBUG("Listing complete");

    QList<newsgroup> list;

    for (const auto& ui : l.groups)
    {
        newsgroup group;
        group.first = ui.first;
        group.last  = ui.last;
        group.size  = ui.size;
        group.name  = from_utf8(ui.name);
        list.append(group);
    }

    emit listingCompleted(l.account, list);
}

engine* g_engine;


} // app
    