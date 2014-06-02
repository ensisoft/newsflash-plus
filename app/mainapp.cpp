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

#include "qtsingleapplication/qtsingleapplication.h"
#include "gui/mainwindow.h"
#include "gui/accounts.h"
#include "gui/dlgwelcome.h"
#include "mainapp.h"
#include "eventlog.h"
#include "settings.h"
#include "utility.h"
#include "format.h"
#include "config.h"

namespace {

using app::str;

void log_copyright_and_versions()
{
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
}

bool load_settings(app::settings& settings, app::eventlog& log, bool& first_launch)
{
    const auto home = QDir::homePath();
    const auto file = home + "/.newsflash/settings.json";

    QFile io(file);    

    DEBUG(str("Settings _1", io));
    try
    {
        if (!io.open(QIODevice::ReadOnly))
        {
            if (!QFile::exists(file))
            {
                first_launch = true;
                //log.write(app::eventlog::event_t type, const QString &msg, const QString &ctx)
                DEBUG("First launch");
            }
            else
            {
                ERROR(str("Failed to open settings _1", io));
                ERROR(str("File error ", io.error()));
            }
            return false;
        }
        settings.load(io);
    }
    catch (const std::exception& e)
    {
        ERROR(str("Failed to load settings _1", io));
        ERROR(e.what());
    }
    INFO("Settings loaded");
    return true;
}

bool save_settings(const app::settings& settings)
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
        settings.save(io);
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

} // namespace

namespace app
{

mainapp::mainapp() : settings_(nullptr), window_(nullptr)
{}

mainapp::~mainapp()
{}

int mainapp::run(int argc, char* argv[])
{   
    app::set_cmd_line(argc, argv);    
    app::settings settings;    
    app::eventlog events;

    // style must be applied before any application
    // object is created in order to aply properly.
    bool first_launch = false;
    if (load_settings(settings, events, first_launch))
    {
        const QString& name = settings.get("settings", "style", "default");
        if (name.toLower() != "default")
        {
            QStyle* style = QApplication::setStyle(name);
            if (style)
                QApplication::setPalette(style->standardPalette());

            events.write(eventlog::event_t::debug,
                str("Selected style '_1'", name), "default");
        }
    }

    QtSingleApplication qtinstance(argc, argv);
    if (qtinstance.isRunning())
    {
        const QStringList& cmds = app::get_cmd_line();
        for (int i=1; i<cmds.size(); ++i)
            qtinstance.sendMessage(cmds[i]);
        return 0;            
    }

    events.hook(qtinstance);

    const auto path = app::get_installation_directory();    

    QCoreApplication::setLibraryPaths(QStringList());
    QCoreApplication::addLibraryPath(path);
    QCoreApplication::addLibraryPath(path + "/plugins-qt");

    log_copyright_and_versions();

    gui::MainWindow window(*this);
    window.show();
    window.apply(settings);
    if (first_launch)
    {
        QTimer::singleShot(500, this, SLOT(welcome_new_user()));

#if defined(LINUX_OS)
        // todo: try to figure out what kind of distro is in question
        settings.set("settings", "open_cmd", "gnome-open");
        settings.set("settings", "shutdown_cmd", "gnome-session-quit --power-off --no-prompt");
#endif

    }

    settings_ = &settings;
    window_   = &window;

    const int ret = qtinstance.exec();

    events.unhook(qtinstance);
    return ret;
}

bool mainapp::shutdown()
{
    window_->persist(*settings_);

    if (!save_settings(*settings_))
        return false;

    // shutdown plugins
    // shutdown extensions
    // shutdown python
    // shutdown engine

    return true;
}

void mainapp::welcome_new_user()
{
    gui::DlgWelcome dlg(window_);
    if (dlg.exec() == QDialog::Accepted)
    {

    }

    if (dlg.open_guide())
    {
        open_help("quick.html");
    }
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
    const auto& cmd = settings_->get("settings", "open_cmd").toString();
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


} // app
