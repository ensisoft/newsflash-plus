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
#  include <QtGui/QStyle>
#  include <QtGui/QMessageBox>
#  include <QtGui/QSystemTrayIcon>
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
#include "newslist.h"
#include "eventlog.h"
#include "rss.h"
#include "nzbfile.h"
#include "nzbcore.h"
#include "downloads.h"
#include "coremodule.h"
#include "toolmodule.h"
#include "linuxmodule.h"
#include "appearance.h"
#include "repair.h"
#include "archives.h"
#include "files.h"
#include "commands.h"
#include "unpack.h"
#include "notify.h"
#include "searchmodule.h"
#include "../debug.h"
#include "../format.h"
#include "../distdir.h"
#include "../homedir.h"
#include "../accounts.h"
#include "../nzbfile.h"
#include "../connlist.h"
#include "../tasklist.h"
#include "../engine.h"
#include "../settings.h"
#include "../tools.h"
#include "../omdb.h"
#include "../version.h"
#include "../eventlog.h"
#include "../repairer.h"
#include "../unpacker.h"
#include "../files.h"
#include "../par2.h"
#include "../unrar.h"
#include "../arcman.h"
#include "../webengine.h"
#include "../poweroff.h"
#include "../platform.h"

namespace gui
{

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

    // initialize home directory.
    app::homedir::init(".newsflash");    

    app::logCopyright();    

    QCoreApplication::setLibraryPaths(QStringList());
    QCoreApplication::addLibraryPath(app::distdir::path());
    QCoreApplication::addLibraryPath(app::distdir::path("plugins-qt"));

    // set the search path for the icons so that the code doesn't explictly
    // refer to a certain size of an icon for example (maybe this will help
    // if icons and their sizes need to be adjusted at runtime on 4k displays
    // for example...
    QDir::setSearchPaths("icons", QStringList(":/resource/16x16_ico_png"));

    // settings is that everything depends on so we must load settings first.
    app::Settings set;

    const auto file = app::homedir::file("settings.json");
    if (QFile::exists(file))
    {
        const auto err = set.load(file);
        if (err != QFile::NoError)
            ERROR("Failed to read settings %1, %2", file, err);
    }

    app::Poweroff power;
    app::g_poweroff = &power;


    // initialize global pointers.
    app::WebEngine web;
    app::g_web = &web;

    app::Engine engine;
    app::g_engine = &engine;

    QObject::connect(&engine, SIGNAL(numPendingTasks(std::size_t)),
        &power, SLOT(numPendingTasks(std::size_t)));

    app::Accounts acc;
    app::g_accounts = &acc;

    app::tools tools;
    app::g_tools = &tools;

    app::MovieDatabase omdb;
    app::g_movies = &omdb;

    // instead of the stack..

     // main application window
    gui::MainWindow win(set);
    gui::g_win = &win;

    // first power module sends initPoweroff signal
    // we ask the MainWindow to close. 
    // After mainwindow is closed.. exit the loop    
    QObject::connect(&power, SIGNAL(initPoweroff()), &win, SLOT(close()));
    QObject::connect(&win, SIGNAL(closed()), &qtinstance, SLOT(quit()));

    // accounts widget
    gui::Accounts gacc;
    win.attach(&gacc);

    // groups widget
    gui::NewsList news;
    win.attach(&news);

    // RSS widget
    gui::RSS rss;
    win.attach(&rss);

    // downloads widget
    gui::Downloads downloads;
    win.attach(&downloads);

    // files component
    app::Files files;    
    gui::Files filesUI(files);
    win.attach(&filesUI);
    // connect to the engine
    QObject::connect(&engine, SIGNAL(fileCompleted(const app::FileInfo&)),
        &files, SLOT(fileCompleted(const app::FileInfo&)));
    QObject::connect(&engine, SIGNAL(packCompleted(const app::FilePackInfo&)),
        &files, SLOT(packCompleted(const app::FilePackInfo&)));

    // repair component
    std::unique_ptr<app::ParityChecker> parityEngine(new app::Par2(
        app::distdir::file("par2")));
    app::Repairer repairer(std::move(parityEngine));
    QObject::connect(&repairer, SIGNAL(repairEnqueue(const app::Archive&)),
        &power, SLOT(repairEnqueue()));
    QObject::connect(&repairer, SIGNAL(repairReady(const app::Archive&)),
        &power, SLOT(repairReady()));

    // connect to the repair GUI
    gui::Repair repairGui(repairer);    

    //  unpack
    std::unique_ptr<app::Archiver> extractEngine(new app::Unrar(
        app::distdir::file("unrar")));
    app::Unpacker unpacker(std::move(extractEngine));
    QObject::connect(&unpacker, SIGNAL(unpackEnqueue(const app::Archive&)),
        &power, SLOT(unpackEnqueue()));
    QObject::connect(&unpacker, SIGNAL(unpackReady(const app::Archive&)),
        &power, SLOT(unpackReady()));

    gui::Unpack unpackGui(unpacker);


    // compose repair + unpack together into a single GUI element
    gui::Archives archives(unpackGui, repairGui);
    win.attach(&archives);

    // archive manager runs the show regarding repair/unpack order.
    app::ArchiveManager arcMan(repairer, unpacker);
    QObject::connect(&engine, SIGNAL(packCompleted(const app::FilePackInfo&)),
        &arcMan, SLOT(packCompleted(const app::FilePackInfo&)));
    QObject::connect(&engine, SIGNAL(fileCompleted(const app::FileInfo&)),
        &arcMan, SLOT(fileCompleted(const app::FileInfo&)));

    QObject::connect(&arcMan, SIGNAL(numPendingArchives(std::size_t)),
        &power, SLOT(numPendingArchives(std::size_t)));


    // eventlog module. this is a bit special
    // because it is used literally from everywhere.
    gui::EventLog log;
    win.attach(&log);    

    // core module
    gui::CoreModule core;
    win.attach(&core);

    // nzb module
    gui::NZBCore nzb;
    win.attach(&nzb);

    // tool module
    gui::ToolModule toolsgui;
    win.attach(&toolsgui);

    // search engine
    gui::SearchModule search;
    win.attach(&search);

    // commands module
    app::Commands cmds;
    QObject::connect(&engine, SIGNAL(packCompleted(const app::FilePackInfo&)),
        &cmds, SLOT(packCompleted(const app::FilePackInfo&)));
    QObject::connect(&engine, SIGNAL(fileCompleted(const app::FileInfo&)),
        &cmds, SLOT(fileCompleted(const app::FileInfo&)));
    QObject::connect(&repairer, SIGNAL(repairReady(const app::Archive&)),
        &cmds, SLOT(repairFinished(const app::Archive&)));
    QObject::connect(&unpacker, SIGNAL(unpackReady(const app::Archive&)),
        &cmds, SLOT(unpackFinished(const app::Archive&)));

    gui::Commands cmdsGui(cmds);
    win.attach(&cmdsGui);

#if defined(LINUX_OS)
    gui::LinuxModule linux;
    win.attach(&linux);
#endif

    gui::Appearance style;
    win.attach(&style);

    gui::Notify notify;
    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        auto& eventLog = app::EventLog::get();

        win.attach(&notify);
        QObject::connect(&engine, SIGNAL(packCompleted(const app::FilePackInfo&)),
            &notify, SLOT(packCompleted(const app::FilePackInfo&)));
        QObject::connect(&engine, SIGNAL(fileCompleted(const app::FileInfo&)),
            &notify, SLOT(fileCompleted(const app::FileInfo&)));
        QObject::connect(&eventLog, SIGNAL(newEvent(const app::Event&)),
            &notify, SLOT(newEvent(const app::Event&)));
        QObject::connect(&repairer, SIGNAL(repairReady(const app::Archive&)),
            &notify, SLOT(repairReady(const app::Archive&)));
        QObject::connect(&unpacker, SIGNAL(unpackReady(const app::Archive&)),
            &notify, SLOT(unpackReady(const app::Archive&)));

        QObject::connect(&notify, SIGNAL(exit()), &win, SLOT(close()));
        QObject::connect(&notify, SIGNAL(minimize()), &win, SLOT(hide()));
        QObject::connect(&notify, SIGNAL(restore()), &win, SLOT(show()));
        DEBUG("Connected notify module.");
    }
    else
    {
        WARN("System tray is not available. Notifications are disabled.");
    }

    win.loadState();
    win.prepareFileMenu();
    win.prepareWindowMenu();
    win.prepareMainTab();
    win.show();

    auto ret = qtinstance.exec();

    // its important to keep in mind that the modules and widgets
    // are created simply on the stack here and what is the actual
    // destruction order. our mainwindow outlives the widgets and modules
    // so before we start destructing those objects its better to make clean
    // all the references to those in the mainwindow.
    win.detachAllWidgets();

    if (power.isPoweroffEnabled())
    {
        DEBUG("Powering off... G'bye");
        app::shutdownComputer();
    }

    return ret;
}

} // gui

int seh_main(int argc, char* argv[])
{
    int ret = 0;

    SEH_BLOCK(ret = gui::run(argc, argv);)

    return ret;
}

int main(int argc, char* argv[])
{
    DEBUG("It's alive!");
    DEBUG("%1 %2", NEWSFLASH_TITLE, NEWSFLASH_VERSION);
    try 
    {
        int ret = seh_main(argc, argv);

        DEBUG("Goodbye...(exitcode: %1)", ret);
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
