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

#define LOGTAG "main"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <boost/version.hpp>
#  include <QtGui/QStyle>
#  include <QtGui/QMessageBox>
#  include <QDir>
#  include <QFile>
#  include <QUrl>
#include <newsflash/warnpop.h>
#include <iostream>
#include <exception>

#include "qtsingleapplication/qtsingleapplication.h"
#include "mainwindow.h"
#include "minidump.h"
#include "config.h"
#include "accounts.h"
#include "groups.h"
#include "eventlog.h"
#include "rss.h"
#include "nzbfile.h"
#include "nzbcore.h"
#include "downloads.h"
#include "coremodule.h"
#include "toolmodule.h"
#include "linuxmodule.h"
#include "appearance.h"
#include "python.h"
#include "files.h"
#include "../eventlog.h"
#include "../debug.h"
#include "../format.h"
#include "../distdir.h"
#include "../homedir.h"
#include "../accounts.h"
#include "../rss.h"
#include "../nzbfile.h"
#include "../connlist.h"
#include "../tasklist.h"
#include "../engine.h"
#include "../settings.h"
#include "../tools.h"
#include "../omdb.h"

using app::str;

namespace gui
{

void copyright()
{
    const auto boost_major    = BOOST_VERSION / 100000;
    const auto boost_minor    = BOOST_VERSION / 100 % 1000;
    const auto boost_revision = BOOST_VERSION % 100;

    INFO(NEWSFLASH_TITLE " " NEWSFLASH_VERSION);
    INFO(QString::fromUtf8(NEWSFLASH_COPYRIGHT));
    INFO(NEWSFLASH_WEBSITE);
    INFO("Compiled: " __DATE__ ", " __TIME__);    
    INFO("Compiler: " COMPILER_NAME);
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
    INFO("Zlib compression library 1.2.5");
    INFO("Copyright (c) 1995-2010 Jean-Loup Gailly & Mark Adler");
    INFO("http://zlib.net");        
}

int run(int argc, char* argv[])
{
    QtSingleApplication qtinstance(argc, argv);
    if (qtinstance.isRunning())
    {
        const QStringList& cmds = QCoreApplication::arguments();
        for (int i=1; i<cmds.size(); ++i)
            qtinstance.sendMessage(cmds[i]);

        QMessageBox msg;
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Information);
        msg.setText("Another instance of " NEWSFLASH_TITLE " is already running.\r\n"
                    "This instance will now exit.");
        msg.exec();
        return 0;
    }

    // this requires QApplication object to be created...
    app::distdir::init();
    app::homedir::init(".newsflash");    

    copyright();    

    QCoreApplication::setLibraryPaths(QStringList());
    QCoreApplication::addLibraryPath(app::distdir::path());
    QCoreApplication::addLibraryPath(app::distdir::path("plugins-qt"));

    // set the search path for the icons so that the code doesn't explictly
    // refer to a certain size of an icon for example (maybe this will help
    // if icons and their sizes need to be adjusted at runtime on 4k displays
    // for example...
    QDir::setSearchPaths("icons", QStringList(":/resource/16x16_ico_png"));

    // settings is that everything depends on 
    // load settings first.
    app::Settings set;

    const auto file = app::homedir::file("settings.json");
    if (QFile::exists(file))
    {
        const auto err = set.load(file);
        if (err != QFile::NoError)
            ERROR(str("Failed to read settings _1, _2", file, err));
    }

    gui::Appearance style;

    app::NetworkManager net;
    app::g_net = &net;

    app::Engine eng;
    app::g_engine = &eng;

    app::Accounts acc;
    app::g_accounts = &acc;

    app::tools tools;
    app::g_tools = &tools;

    app::MovieDatabase omdb;
    app::g_movies = &omdb;

    // todo: maybe create the widgets and modules on the free store
    // instead of the stack..

     // main application window
    gui::MainWindow win(set);
    gui::g_win = &win;

    // accounts widget
    gui::Accounts gacc;
    win.attach(&gacc);

    // groups widget
    gui::Groups news;
    win.attach(&news);

    // RSS widget
    gui::RSS rss;
    win.attach(&rss);

    // downloads widget
    gui::Downloads downloads;
    win.attach(&downloads);

    // scripting
#if defined(NEWSFLASH_ENABLE_PYTHON)
    gui::Python py;
    win.attach(&py);
#endif

    // eventlog widget
    gui::EventLog log;
    win.attach(&log);

    // files widget
    gui::Files files;
    win.attach(&files);



    // core module
    gui::CoreModule core;
    win.attach(&core);

    // nzb module
    gui::NZBCore nzb;
    win.attach(&nzb);

    // tool module
    gui::ToolModule toolsgui;
    win.attach(&toolsgui);

#if defined(LINUX_OS)
    gui::LinuxModule linux;
    win.attach(&linux);
#endif

    win.attach(&style);

    win.loadState();
    win.prepareFileMenu();
    win.prepareMainTab();
    win.show();

    auto ret = qtinstance.exec();

    // its important to keep in mind that the modules and widgets
    // are created simply on the stack here and what is the actual
    // destruction order. our mainwindow outlives the widgets and modules
    // so before we start destructing those objects its better to make clean
    // all the references to those in the mainwindow.
    win.detachAllWidgets();

    return ret;
}

} // gui

int main(int argc, char* argv[])
{
    DEBUG("It's alive!");
    DEBUG(NEWSFLASH_TITLE << NEWSFLASH_VERSION);
    try 
    {
        int ret = 0;
        SEH_BLOCK(ret = gui::run(argc, argv));

        DEBUG(str("Goodbye...(_1)", ret));
        return ret;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        std::cerr << std::endl;

// Todo: this doesn't work fix it
        QMessageBox msg;
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Critical);
        msg.setText(
            QString("%1 has encountered an error and will now close.\r\n %2")
            .arg(NEWSFLASH_TITLE)
            .arg(e.what()));
        msg.exec();
    }
    return 1;
}