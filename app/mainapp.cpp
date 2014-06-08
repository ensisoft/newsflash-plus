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
#include <vector>
#include "mainapp.h"
#include "datastore.h"
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
    DEBUG("Application created");

    // dispatch_.listen<sdk::cmd_new_account>();
    // dispatch_.listen<sdk::cmd_del_account>();
    // dispatch_.listen<sdk::cmd_set_account>();
    // dispatch_.listen<sdk::cmd_get_account>();
    // dispatch_.listen<sdk::cmd_get_account_quota>();
    // dispatch_.listen<sdk::cmd_set_account_quota>();
    // dispatch_.listen<sdk::cmd_get_account_volume>();
    // dispatch_.listen<sdk::cmd_set_account_volume>();
    // dispatch_.listen<sdk::cmd_shutdown>();

    const auto& home = QDir::homePath();
    const auto& file = home + "/.newsflash/settings.json";

    if (QFile::exists(file))
    {
        QFile io(file);
        if (!io.open(QIODevice::ReadWrite))
        {
            ERROR(str("Failed to open _1", io));
        }
        else
        {
            settings_.load(io);
            accounts_.load(settings_);
            groups_.load(settings_);
        }
    }
    else
    {
       settings_.set("settings", "logs", QDir::toNativeSeparators(home + "/Newsflash/Logs"));
       settings_.set("settings", "data", QDir::toNativeSeparators(home + "/Newsflash/Data"));
       settings_.set("settings", "downloads", QDir::toNativeSeparators(home + "/Newsflash/Downloads"));
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
        WARN(str("Failed to create log path _1. Log files will not be available", dir));
    }

    engine_.reset(new newsflash::engine(*this, utf8(logs)));

    app.installEventFilter(this);
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

// void mainapp::react(std::shared_ptr<sdk::command> cmd)
// {
//     if (!dispatch_.dispatch(cmd))
//     {
//         const auto* type = sdk::command_register::find(cmd->identity());
//         const auto* name = type->name();
//         qFatal("No handler for '%s', is this intentional??", name);
//     }
// }

// void mainapp::on_command(std::shared_ptr<sdk::cmd_new_account>& cmd)
// {
//     const auto& acc = accounts_.suggest();

//     copy_account(*cmd, acc);

//     cmd->accept();
// }

// void mainapp::on_command(std::shared_ptr<sdk::cmd_del_account>& cmd)
// {
//     accounts_.del(cmd->index);

//     cmd->accept();
// }

// void mainapp::on_command(std::shared_ptr<sdk::cmd_set_account>& cmd)
// {
//     accounts::account acc = {};
//     copy_account(acc, *cmd);

//     accounts_.set(acc);
//     accounts_.save(settings_);

//     // configure engine with this account
//     newsflash::account my_account;
//     my_account.id                    = acc.id;
//     my_account.name                  = narrow(acc.name);
//     my_account.username              = utf8(acc.username);
//     my_account.password              = utf8(acc.password);
//     my_account.secure_host           = latin(acc.secure_host);
//     my_account.secure_port           = acc.secure_port;
//     my_account.general_host          = latin(acc.general_host);
//     my_account.general_port          = acc.general_port;
//     my_account.connections           = acc.connections;
//     my_account.enable_secure_server  = acc.enable_secure_server;
//     my_account.enable_general_server = acc.enable_general_server;
//     my_account.enable_compression    = acc.enable_compression;
//     my_account.enable_pipelining     = acc.enable_pipelining;

//     // todo: look this up in the sttings!
//     //my_account.is_fill_account = todo:

//     engine_->set(my_account);

//     cmd->accept();
// }

// void mainapp::on_command(std::shared_ptr<sdk::cmd_get_account>& cmd)
// {
//     const auto& acc = accounts_.get(cmd->index);

//     copy_account(*cmd, acc);

//     cmd->accept();
// }

// void mainapp::on_command(std::shared_ptr<sdk::cmd_get_account_quota>& cmd)
// {
//     const auto& acc = accounts_.get(cmd->index);

//     cmd->available = acc.quota_avail;
//     cmd->consumed  = acc.quota_spent;
//     cmd->enabled   = acc.enable_quota;
//     if (acc.quota_type == accounts::quota::fixed)
//         cmd->monthly = false;
//     else cmd->monthly = true;

//     cmd->accept();
// }

// void mainapp::on_command(std::shared_ptr<sdk::cmd_set_account_quota>& cmd)
// {
//     auto& acc = accounts_.get(cmd->index);

//     acc.enable_quota = cmd->enabled;
//     acc.quota_avail  = cmd->available;
//     acc.quota_spent  = cmd->consumed;
//     if (cmd->monthly)
//         acc.quota_type = accounts::quota::monthly;
//     else acc.quota_type = accounts::quota::fixed;

//     cmd->accept();
// }

// void mainapp::on_command(std::shared_ptr<sdk::cmd_get_account_volume>& cmd)
// {
//     const auto& acc = accounts_.get(cmd->index);
//     cmd->this_month = acc.downloads_this_month;
//     cmd->all_time   = acc.downloads_all_time;
//     cmd->accept();
// }

// void mainapp::on_command(std::shared_ptr<sdk::cmd_set_account_volume>& cmd)
// {
//     cmd->accept();
// }

// void mainapp::on_command(std::shared_ptr<sdk::cmd_shutdown>& cmd)
// {
//     const auto& home = QDir::homePath();
//     const auto& file = home + "/.newsflash/settings.json";
//     QFile io(file);
//     if (!io.open(QIODevice::WriteOnly))
//     {
//         ERROR(str("Failed to open settings file _1", io));        
//         if (!cmd->ignore_errors)
//         {
//             cmd->fail();
//             return;
//         }
//     }
//     else
//     {
//         accounts_.save(settings_);
//         settings_.save(io);

//         DEBUG(str("Settings saved _1", io));    

//         io.flush();
//         io.close();
//     }

//     if (!engine_->is_running())
//     {
//         cmd->accept();
//         return;
//     }    

//     // we take a snapshot of engine state (downloads)
//     // and store them in a file so that we can restore the 
//     // engine downloads on next application run. if UI is forcing
//     // us to close we ignore any errors that might occur
//     // in the process.

//     // todo:

 
// }

} // app
