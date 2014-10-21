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

#define LOGTAG "app"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QCoreApplication>
#  include <QEvent>
#  include <QFile>
#  include <QDir>
#  include <QBuffer>
#include <newsflash/warnpop.h>
#include <newsflash/engine/account.h>

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
    // engine_.on_error = std::bind(&engine::on_engine_error, this,
    //     std::placeholders::_1);
    // engine_.on_file  = std::bind(&engine::on_engine_file, this,
    //     std::placeholders::_1);

    // engine_.on_async_notify = [=]() {
    //     QCoreApplication::postEvent(this, new AsyncNotifyEvent);
    // };

    // settings_.auto_connect             = true;
    // settings_.auto_remove_completed    = false;
    // settings_.discard_text_content     = true;
    // settings_.enable_fill_account      = false;
    // settings_.enable_throttle          = false;
    // settings_.fill_account             = 0;
    // settings_.overwrite_existing_files = false;
    // settings_.prefer_secure            = true;
    // settings_.throttle                 = 0;
    // engine_.configure(settings_);

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
    a.name                  = utf8(acc.name);
    a.username              = utf8(acc.username);
    a.password              = utf8(acc.password);
    a.secure_host           = latin(acc.secure_host);
    a.secure_port           = acc.secure_port;    
    a.general_host          = latin(acc.general_host);
    a.general_port          = acc.general_port;    
    a.enable_compression    = acc.enable_compression;
    a.enable_pipelining     = acc.enable_pipelining;
    a.enable_general_server = acc.enable_general_server;
    a.enable_secure_server  = acc.enable_secure_server;
    //engine_.set(a);
}

void engine::del(const account& acc)
{
    // todo:
}

void engine::set_fill_account(quint32 id)
{}

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

    QDir dir;
    if (!dir.mkpath(path))
    {
        ERROR(str("Error creating path _1", dir));
        return;
    }

    std::vector<newsflash::ui::download> jobs;
    for (auto& item : items)
    {
        newsflash::ui::download download;
        download.articles = std::move(item.segments);
        download.groups   = std::move(item.groups);
        download.size     = item.bytes;
        download.path     = utf8(path);
        download.desc     = utf8(desc);
        jobs.push_back(std::move(download));
    }
    engine_.download(acc, std::move(jobs));

    INFO(str("Downloading _1", desc));
}

bool engine::eventFilter(QObject* object, QEvent* event)
{
    if (object == this && event->type() == AsyncNotifyEvent::identity())
    {
        //engine_.pump();
        return true;
    }
    return QObject::eventFilter(object, event);
}

// void engine::on_engine_error(const newsflash::ui::error& e)
// {}

// void engine::on_engine_file(const newsflash::ui::file& f)
// {}


engine* g_engine;


} // app
