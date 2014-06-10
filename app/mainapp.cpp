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
#  include <boost/version.hpp>
#  include <QCoreApplication>
#  include <QStringList>
#  include <QFile>
#  include <QDir>
#  include <QEvent>
#include <newsflash/warnpop.h>

#include <newsflash/engine/engine.h>
#include <newsflash/engine/account.h>
#include <newsflash/engine/listener.h>
#include <newsflash/sdk/format.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/home.h>
#include <vector>
#include "mainapp.h"
#include "accounts.h"
#include "groups.h"

using sdk::str;
using sdk::str_a;
using sdk::utf8;
using sdk::narrow;
using sdk::utf8;
using sdk::latin;

namespace {
    // we post this event object to the application's event queue
    // when a thread in the engine calls async_notify.
    // mainapp then installs itself as a global event filter
    // and uses the filter function to pick up the notification event
    // to drive the engine's event pump
    class async_notify_event : public QEvent 
    {
    public:
        async_notify_event() : QEvent(identity())
        {}

        static 
        QEvent::Type identity() 
        {
            static auto id = QEvent::registerEventType();
            return (QEvent::Type)id;
        }
    private:
    };

} // namespace

namespace app
{

mainapp::mainapp(QCoreApplication& app) : app_(app)
{
    const auto file  = sdk::home::file("engine.json");
    if (QFile::exists(file))
    {
        const auto error = settings_.load(file);
        if (error != QFile::NoError)
        {
            ERROR(str("Failed to read settings _1, _2", error, file));
        }
        else
        {
            accounts_.load(settings_);
            groups_.load(settings_);        
        }
    }
    else
    {
//       settings_.set("settings", "logs", QDir::toNativeSeparators(home + "/Newsflash/Logs"));
//       settings_.set("settings", "data", QDir::toNativeSeparators(home + "/Newsflash/Data"));
//       settings_.set("settings", "downloads", QDir::toNativeSeparators(home + "/Newsflash/Downloads"));
       settings_.set("settings", "discard_text_content", true);
       settings_.set("settings", "overwrite_existing", false);
       settings_.set("settings", "remove_complete", false);
       settings_.set("settings", "prefer_secure", true);
       settings_.set("settings", "enable_throttle", false);
       settings_.set("settings", "throttle", 0);
    }

    const auto& logs = settings_.get("settings", "logs").toString();

    QDir dir(logs);
    if (!dir.mkpath(logs))
    {
        ERROR(str("Failed to create log path _1.", dir));
    }

    engine_.reset(new newsflash::engine(*this, utf8(logs)));

    app.installEventFilter(this);

    DEBUG("Application created");    
}

mainapp::~mainapp()
{
    app_.removeEventFilter(this);

    DEBUG("Application deleted");
}

sdk::model& mainapp::get_model(const QString& name)
{
    if (name == "eventlog")
        return sdk::eventlog::get();
    else if (name == "accounts")
        return accounts_;
    else if (name == "groups")
        return groups_;

    throw std::runtime_error("no such model");
}

bool mainapp::savestate()
{
    const auto file  = sdk::home::file("engine.json");
    const auto error = settings_.save(file);
    if (error != QFile::NoError)
    {
        ERROR(str("Error saving settings _1 _2", error, file));
        return false;
    }
    
    // todo: save engine state

    return true;
}

void mainapp::shutdown()
{
    engine_->stop();
}

void mainapp::handle(const newsflash::error& error)
{}


void mainapp::acknowledge(const newsflash::file& file)
{}


void mainapp::async_notify()
{
    const auto& tid = std::this_thread::get_id();
    std::stringstream ss;
    std::string s;
    ss << tid;
    ss >> s;

    DEBUG(str("Async notify by thread: _1", s));

    // postEvent is a thread safe function  so we're all se here.
    QCoreApplication::postEvent(this, new async_notify_event());
}

bool mainapp::eventFilter(QObject* object, QEvent* event)
{
    if (object != this)
        return false;

    if (event->type() != async_notify_event::identity())
        return false;

    engine_->pump();
    return true;

}



} // app
