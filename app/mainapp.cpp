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

#include <newsflash/config.h>

#include <newsflash/keygen/keygen.h>

#include <newsflash/warnpush.h>
#  include <boost/version.hpp>
#  include <QStringList>
#  include <QFile>
#  include <QDir>
#  include <QTimer>
#include <newsflash/warnpop.h>

#include <Python.h>

#include <newsflash/engine/engine.h>
#include <newsflash/engine/account.h>
#include <newsflash/engine/listener.h>

#include <vector>

#include "qtsingleapplication/qtsingleapplication.h"
#include "gui/mainwindow.h"
#include "gui/accounts.h"
#include "gui/eventlog.h"
#include "gui/dlgwelcome.h"
#include "gui/dlgaccount.h"
#include "mainapp.h"
#include "eventlog.h"
#include "valuestore.h"
#include "utility.h"
#include "format.h"
#include "config.h"
#include "accounts.h"
#include "debug.h"

namespace app
{

mainapp::mainapp() : valuestore_(nullptr), gui_window_(nullptr), virgin_(false)
{}

mainapp::~mainapp()
{}

int mainapp::run(int argc, char* argv[])
{   
    app::set_cmd_line(argc, argv);    

    app::valuestore valuestore;    
    app::eventlog events;
    valuestore_ = &valuestore;
    eventlog_   = &events;

    // we must load valuestore and style information as the first thing
    load_valuestore();

    QtSingleApplication qtinstance(argc, argv);
    if (qtinstance.isRunning())
    {
        const QStringList& cmds = app::get_cmd_line();
        for (int i=1; i<cmds.size(); ++i)
            qtinstance.sendMessage(cmds[i]);
        return 0;            
    }

    // enable this eventlog as the global eventlog
    events.hook(qtinstance);

    const auto boost_major    = BOOST_VERSION / 100000;
    const auto boost_minor    = BOOST_VERSION / 100 % 1000;
    const auto boost_revision = BOOST_VERSION % 100;

    INFO(NEWSFLASH_TITLE " " NEWSFLASH_VERSION);
    INFO(__DATE__ ", " __TIME__);
    INFO(QString::fromUtf8(NEWSFLASH_COPYRIGHT));
    INFO(NEWSFLASH_WEBSITE);
    INFO(str("Boost software library _1._2._3", boost_major, boost_minor, boost_revision));
    INFO("http://www.boost.org");
    INFO("16x16 Free Application Icons");
    INFO("Copyright (c) 2009 Aha-Soft");
    INFO("http://www.small-icons.com/stock-icons/16x16-free-application-icons.htm");
    INFO("http://www.aha-soft.com");
    INFO("Silk Icon Set 1.3");
    INFO("Copyright (c) Mark James");
    INFO("http://www.famfamfam.com/lab/icons/silk/");
    INFO(str("Qt cross-platform application and UI framework _1", QT_VERSION_STR));
    INFO("http://qt.nokia.com");
    INFO(str("Python _1", Py_GetVersion()));
    INFO("http://www.python.org");
    INFO("Zlib compression library 1.2.5");
    INFO("Copyright (c) 1995-2010 Jean-Loup Gailly & Mark Adler");
    INFO("http://zlib.net");        

    const auto path = app::get_installation_directory();    
    QCoreApplication::setLibraryPaths(QStringList());
    QCoreApplication::addLibraryPath(path);
    QCoreApplication::addLibraryPath(path + "/plugins-qt");

    app::accounts accounts;
    accounts_ = &accounts;
    accounts.retrieve(valuestore);

    // open the builtin views    
    gui::MainWindow   gui_window   { *this };
    gui::Accounts     gui_accounts { accounts };
    gui::Eventlog     gui_events   { events };

    gui_window_ = &gui_window;

    views_ = {
        &gui_accounts, 
        &gui_events,
    };

    compose_views();

    gui_window.configure(valuestore);
    gui_window.show();

    // enter event lop
    qtinstance.exec();

    events.unhook(qtinstance);
    return 0;
}

bool mainapp::shutdown()
{
    gui_window_->persist(*valuestore_);
    accounts_->persist(*valuestore_);

    for (auto& ui : views_)
    {
        const auto show = ui->isVisible();
        const auto key  = ui->windowTitle();
        valuestore_->set("visible_tabs", key, show);
    }

    if (!save_valuestore())
        return false;

    // shutdown plugins
    // shutdown extensions
    // shutdown python
    // shutdown engine

    return true;
}

void mainapp::handle(const newsflash::error& error)
{
    // todo:
}

void mainapp::acknowledge(const newsflash::file& file)
{
    // todo:
}

void mainapp::notify() 
{
    // todo:
}

void mainapp::welcome_new_user()
{
    gui::DlgWelcome dlg(gui_window_);
    if (dlg.exec() == QDialog::Accepted)
    {
        auto acc = accounts_->create();
        gui::DlgAccount dlg(gui_window_, acc, true);
        if (dlg.exec() == QDialog::Accepted)
            accounts_->set(acc);
    }

    if (dlg.open_guide())
    {
        open_help("quick.html");
    }
}

void mainapp::modify_account(std::size_t i)
{
    const auto& account = accounts_->get(i);
//    DEBUG(str("Account was edited '_1'", account.name));

    // update engine configuration

    newsflash::account na;
    na.id                 = account.id;
    na.enable_compression = account.enable_compression;
    na.enable_pipelining  = account.enable_pipelining;
    na.is_fill_account    = false; // todo:
    na.max_connections    = account.maxconn;
    na.name               = utf8(account.name);
    na.username           = utf8(account.user);
    na.password           = utf8(account.pass);
    if (account.general.enabled)
    {
        na.general.host = utf8(account.general.host);
        na.general.port = account.general.port;
    }
    if (account.secure.enabled)
    {
        na.secure.host = utf8(account.secure.host);
        na.secure.port = account.secure.port;
    }

    // modify engine account now
   // engine_->set(na);
}

bool mainapp::open(const QString& resource)
{
#if defined(WINDOWS_OS)
    const auto ret = ShellExecute(
        NULL,
        L"open",
        resource.utf16(),
        NULL, // parameters dont care
        NULL, // working directory
        SW_SHOWNORMAL);
    if ((int)ret <= 32)
    {
        ERROR(err(GetLastError()));
        return false;
    }
    return true;

#elif defined(LINUX_OS)
    const auto& cmd = valuestore_->get("settings", "open_cmd").toString();
    if (cmd.isEmpty())
    {
        WARN(str("Open command is not set. Unable to open _1", resource));
        return false;
    }

    return true;
#endif
}

bool mainapp::open_help(const QString& page)
{
    return open(QDir::toNativeSeparators(
        app::get_installation_directory() + "/help" + page));
}

bool mainapp::load_valuestore()
{
// #define E(m) \
//     eventlog_->write(eventlog::event_t::error, m, "default");
// #define I(m) \
//     eventlog_->write(eventlog::event_t::info, m, "default");    

    const auto& home = QDir::homePath();
    const auto& file = home + "/.newsflash/settings.json";
    const auto& logs = home + "/Newsflash/Logs";
    const auto& bins = home + "/Newsflash/Downloads";    

    if (!QFile::exists(file))
    {
        DEBUG("First launch");
        valuestore_->set("settings", "logs", logs);
        valuestore_->set("settings", "downloads", bins);
#if defined(LINUX_OS)
        valuestore_->set("settings", "open_cmd", "gnome-open");
        valuestore_->set("settings", "shutdown_cmd", "gnome-session-quit --power-off --no-prompt");
#endif
        QTimer::singleShot(500, this, SLOT(welcome_new_user()));
        return false;
    }

    QFile io(file);
    if (!io.open(QIODevice::ReadOnly))
    {
        //E(str("Failed to open valuestore _1", io));
        //E(str("File error ", io.error()));
        return false;
    }
    valuestore_->load(io);

    // style must be applied as the first thing,
    // before any Q application object is created
    // in order to apply properly
    const auto& name = valuestore_->get("settings", "style", "default");
    if (name != "default")
    {
        QStyle* style = QApplication::setStyle(name);
        if (style)
        {
            QApplication::setPalette(style->standardPalette());
        }
    }

    return true;

#undef D
#undef E
#undef I

}

bool mainapp::save_valuestore()
{
    const auto home = QDir::homePath();
    const auto path = home + "/.newsflash";
    const auto file = home + "/.newsflash/settings.json";

    QFile io(file);

    DEBUG(str("Settings _1", file));
    try
    {
        QDir dir(path);
        if (!dir.mkpath(path))
        {
            ERROR(str("Failed to save settings. Make directory failed _1", dir));
            return false;
        }

        if (!io.open(QIODevice::WriteOnly))
        {
            ERROR(str("Failed to save settings _1", io));
            ERROR(str("File error ", io.error()));
            return false;
        }
        valuestore_->save(io);
    }
    catch (const std::exception& e)
    {
        ERROR(str("Failed to save settings _1", io));
        ERROR(e.what());
        return false;
    }
    INFO("Settings saved");
    return true;
}

void mainapp::compose_views()
{
    for (auto& ui : views_)
    {
        gui_window_->compose(ui);
        const auto& key  = ui->windowTitle();
        const auto& info = ui->get_info();
        const bool show  = valuestore_->get("visible_tabs", key, info.visible_by_default);
        if (show)
        {
            gui_window_->show(ui);            
        }
        DEBUG(str("_1 is visible _2", key, show));
    }
}

} // app
