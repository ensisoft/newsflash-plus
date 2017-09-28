// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define LOGTAG "main"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtGui/QStyle>
#  include <QtGui/QMessageBox>
#  include <QtGui/QSystemTrayIcon>
#  include <QDir>
#  include <QFile>
#  include <QUrl>
#  include <QImageReader>
#include "newsflash/warnpop.h"
#include <iostream>
#include <exception>

#include "engine/minidump.h"
#include "engine/engine.h"
#include "engine/logging.h"

#include "qtsingleapplication/qtsingleapplication.h"
#include "mainwindow.h"
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
#include "historydb.h"
#include "omdb.h"
#include "filesystem.h"
#include "app/debug.h"
#include "app/format.h"
#include "app/distdir.h"
#include "app/homedir.h"
#include "app/accounts.h"
#include "app/nzbfile.h"
#include "app/connlist.h"
#include "app/tasklist.h"
#include "app/engine.h"
#include "app/settings.h"
#include "app/tools.h"
#include "app/omdb.h"
#include "app/version.h"
#include "app/eventlog.h"
#include "app/repairer.h"
#include "app/unpacker.h"
#include "app/files.h"
#include "app/par2.h"
#include "app/unrar.h"
#include "app/unzip.h"
#include "app/arcman.h"
#include "app/webengine.h"
#include "app/poweroff.h"
#include "app/platform.h"
#include "app/historydb.h"
#include "app/media.h"
#include "app/filetype.h"

namespace gui
{

int run(QtSingleApplication& qtinstance)
{
    // initialize home directory.
    app::homedir::init(".newsflash");

    newsflash::initialize();

    app::logCopyright();

    const auto& args = qtinstance.arguments();
    for (const auto& arg : args)
    {
        if (arg == "--debug")
        {
            newsflash::enable_debug_log(true);
            INFO("Debug log enabled");
            break;
        }
    }

    QCoreApplication::setLibraryPaths(QStringList());
    QCoreApplication::addLibraryPath(app::distdir::path());
    QCoreApplication::addLibraryPath(app::distdir::path("plugins-qt"));

    // set the search path for the icons so that the code doesn't explictly
    // refer to a certain size of an icon for example (maybe this will help
    // if icons and their sizes need to be adjusted at runtime on 4k displays
    // for example...
    QDir::setSearchPaths("icons", QStringList(":/resource/16x16_ico_png"));

    const auto& formats = QImageReader::supportedImageFormats();
    for (const auto& format : formats)
    {
        DEBUG("ImageFormat: %1", format);
    }

    // settings is that everything depends on so we must load settings first.
    app::Settings settings;

    const auto file = app::homedir::file("settings.json");
    if (QFile::exists(file))
    {
        const auto err = settings.load(file);
        if (err != QFile::NoError)
            ERROR("Failed to read settings %1, %2", file, err);
    }

    if (!settings.contains("version", "mediatype"))
        settings.set("version", "mediatype", 1);

    if (!settings.contains("version", "filetype"))
        settings.set("version", "filetype", 1);

    // initialize global pointers.
    app::Poweroff power;
    app::g_poweroff = &power;

    app::HistoryDb hdb;
    app::g_history = &hdb;

    app::WebEngine web;
    app::g_web = &web;

    app::Engine engine;
    app::g_engine = &engine;

    QObject::connect(&engine, SIGNAL(numPendingTasks(std::size_t)),
        &power, SLOT(numPendingTasks(std::size_t)));
    QObject::connect(&engine, SIGNAL(newDownloadQueued(const Download&)),
        &hdb, SLOT(newDownloadQueued(const Download&)));

    app::Accounts acc;
    app::g_accounts = &acc;

    app::tools tools;
    app::g_tools = &tools;

    app::MovieDatabase omdb;
    app::g_movies = &omdb;

     // main application window
    gui::MainWindow win(settings);
    gui::g_win = &win;
    QObject::connect(&qtinstance, SIGNAL(messageReceived(const QString&)),
        &win, SLOT(messageReceived(const QString&)));


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
    app::Unpacker unpacker;
    unpacker.addEngine(std::make_unique<app::Unrar>(app::distdir::file("unrar")));
    unpacker.addEngine(std::make_unique<app::Unzip>(app::distdir::file("7za")));
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

    gui::HistoryDb historygui(app::g_history);
    win.attach(&historygui);

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
    #undef linux
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

        QObject::connect(&win, SIGNAL(shown()), &notify, SLOT(windowRestored()));
        DEBUG("Connected notify module.");
    }
    else
    {
        WARN("System tray is not available. Notifications are disabled.");
    }

    gui::Omdb omdbgui(&omdb);
    win.attach(&omdbgui);

    gui::FileSystemModule fileSysMod;
    win.attach(&fileSysMod);

    app::g_accounts->loadState(settings);
    app::g_tools->loadState(settings);
    app::g_engine->loadState(settings);
    app::g_history->loadState(settings);
    app::g_history->loadHistory();

    win.loadState();
    win.prepareFileMenu();
    win.prepareWindowMenu();
    win.prepareMainTab();
    win.startup();
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

int seh_main(QtSingleApplication& qtinstance)
{
    int ret = 0;

    SEH_BLOCK(ret = gui::run(qtinstance);)

    return ret;
}

int main(int argc, char* argv[])
{
    DEBUG("It's alive!");
    DEBUG("%1 %2", NEWSFLASH_TITLE, NEWSFLASH_VERSION);

    QtSingleApplication qtinstance(argc, argv);
    if (qtinstance.isRunning())
    {
        const QStringList& cmds = QCoreApplication::arguments();
        for (int i=1; i<cmds.size(); ++i)
            qtinstance.sendMessage(cmds[i]);

        if (cmds.size() == 1)
        {
            QMessageBox msg;
            msg.setStandardButtons(QMessageBox::Ok);
            msg.setIcon(QMessageBox::Information);
            msg.setText("Another instance of " NEWSFLASH_TITLE " is already running.\r\n"
                "This instance will now exit.");
            msg.exec();
        }
        return 0;
    }

    try
    {
        int ret = seh_main(qtinstance);

        DEBUG("Goodbye...(exitcode: %1)", ret);
        return ret;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        std::cerr << std::endl;

        QMessageBox msg;
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Critical);
        msg.setText(
            QString("%1 has encountered an unhandled exception and will now close.\r\n %2")
            .arg(NEWSFLASH_TITLE)
            .arg(e.what()));
        msg.exec();
    }
    return 1;
}
