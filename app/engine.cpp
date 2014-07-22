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
#include <newsflash/warnpop.h>
//#include <newsflash/engine/engine.h>
//#include <newsflash/engine/listener.h>
//#include <newsflash/engine/settings.h>
//#include <newsflash/engine/account.h>

#include "engine.h"
#include "debug.h"
#include "datastore.h"
#include "home.h"
#include "format.h"
#include "eventlog.h"
#include "account.h"

namespace {
    class notify : public QEvent
    {
    public:
        notify() : QEvent(identity())
        {}

        static 
        QEvent::Type identity() {
            static auto id = QEvent::registerEventType();
            return (QEvent::Type)id;
        }
    private:
    };
} // namespace

namespace app
{

engine* g_engine;

// class engine::listener : public newsflash::listener
// {
// public:
//     listener(app::engine* eng) : engine_(eng)
//     {}

//     virtual void handle(const newsflash::error& error) override
//     {}

//     virtual void acknowledge(const newsflash::file& file) override
//     {

//     }
//     // this callback gets invoked by a thread in the engine when there 
//     // are new pending/queued events that need processing. 
//     // this is an asynchronous callback and can occur from multiple threads.
//     virtual void async_notify() override
//     {
//         QCoreApplication::postEvent(engine_, new notify());
//     }
// private:
//     friend class app::engine;
// private:
//     app::engine* engine_;
// };

engine::engine(QCoreApplication& application) : app_(application)
{
    datastore data;
    const auto file = home::file("engine.json");
    if (QFile::exists(file))
    {
        const auto error = data.load(file);
        if (error != QFile::NoError)
        {
            ERROR(str("Failed to read engine settings _1, _2", file, error));
        }
    }
    // settings_.reset(new newsflash::settings);
    // settings_->overwrite_existing_files = data.get("engine", "overwrite_existing_files", false);
    // settings_->discard_text_content     = data.get("engine", "discard_text_content", false);
    // settings_->auto_remove_completed    = data.get("engine", "remove_complete", false);
    // settings_->prefer_secure            = data.get("engine", "prefer_secure", true);
    // settings_->enable_throttle          = data.get("engine", "enable_throttle", false);
    // settings_->enable_fillserver        = data.get("engine", "enable_fillserver", false);
    // settings_->throttle                 = data.get("engine", "throttle", 0);

    QString logs = data.get("engine", "log_files", "");
    if (logs.isEmpty())
    {
#if defined(LINUX_OS)
        // we use /tmp instead of /var/logs because most likely
        // we don't have the permission to write into /var/logs
        logs = "/tmp/newsflash";
#elif defined(WINDOWS_OS)

#endif
    }

    DEBUG(str("Engine log files will be stored in _1", logs));

    QDir dir(logs);
    if (!dir.mkpath(logs))
    {
        ERROR(str("Failed to create log folder _1. Log files will not be available", dir));            
    }

    //listener_.reset(new listener(this));
    //engine_.reset(new newsflash::engine(*listener_, utf8(logs)));
    //engine_->set(*settings_);

    app_.installEventFilter(this);

    DEBUG("Engine created");
}

engine::~engine()
{
    app_.removeEventFilter(this);

    DEBUG("Engine destroyed");
}

void engine::set(const account& acc)
{
    // newsflash::account a;
    // a.id                    = acc.id();
    // a.name                  = utf8(acc.name());
    // a.username              = utf8(acc.username());
    // a.password              = utf8(acc.password());
    // a.secure_host           = latin(acc.secure_host());
    // a.secure_port           = acc.secure_port();
    // a.general_host          = latin(acc.general_host());
    // a.general_port          = acc.general_port();
    // a.connections           = acc.connections();
    // a.enable_secure_server  = acc.enable_secure_server();
    // a.enable_general_server = acc.enable_general_server();
    // a.enable_compression    = acc.enable_compression();    
    // a.enable_pipelining     = acc.enable_pipelining();
    // engine_->set(a);

    DEBUG(str("Configure engine with account _1", acc.name()));
}

bool engine::eventFilter(QObject* object, QEvent* event)
{
    if (object == this  &&
        event->type() == notify::identity())
    {
        //engine_->pump();
        return true;
    }
    return QObject::eventFilter(object, event);
}

} // app
