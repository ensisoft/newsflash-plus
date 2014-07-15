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

#define LOGTAG "gui"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <boost/version.hpp>
#  include <QtGui/QStyle>
#  include <QtGui/QMessageBox>
#  include <QtGui/QDesktopServices>
#  include <QDir>
#  include <QFile>
#  include <QUrl>
#include <newsflash/warnpop.h>

#include <iostream>
#include <exception>

#include <Python.h>

#include "qtsingleapplication/qtsingleapplication.h"
#include "mainwindow.h"
#include "minidump.h"
#include "config.h"
#include "accounts.h"
#include "groups.h"
#include "eventlog.h"
#include "rss.h"
#include "nzbfile.h"
#include "downloads.h"
#include "../mainapp.h"
#include "../eventlog.h"
#include "../debug.h"
#include "../format.h"
#include "../dist.h"
#include "../home.h"
#include "../datastore.h"
#include "../accounts.h"
#include "../groups.h"
#include "../rss.h"
#include "../nzbfile.h"
#include "../connlist.h"
#include "../tasklist.h"
#include "../engine.h"

using app::str;

namespace gui
{

void openurl(const QString& url)
{
    QDesktopServices::openUrl(url);
}

void openhelp(const QString& page)
{
    const auto& help = app::dist::path("help");
    const auto& file = QString("file://%1/%2").arg(help).arg(page);
    QDesktopServices::openUrl(file);
}

// note that any Qt style must be applied first
// thing before any Qt application object is instance is created.
void setstyle()
{
    const auto& file = app::home::file("style");
    if (!QFile::exists(file))
        return;

    QFile io(file);
    QTextStream in(&io);
    QString name = in.readLine();
    if (name.isEmpty())
        return;

    if (name.toLower() == "default")
        return;

    DEBUG(str("Qt style _1", name));    

    QStyle* style = QApplication::setStyle(name);
    if (style)
        QApplication::setPalette(style->standardPalette());

}

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
    //INFO(str("Python _1", Py_GetVersion()));
    INFO("Python 2.5.1");
    INFO("http://www.python.org");
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


    app::home::init(".newsflash");
    app::dist::init();

    setstyle();
    copyright();    

    QCoreApplication::setLibraryPaths(QStringList());
    QCoreApplication::addLibraryPath(app::dist::path());
    QCoreApplication::addLibraryPath(app::dist::path("plugins-qt"));

    app::engine eng(qtinstance);
    app::g_engine = &eng;

    // main application module
    app::mainapp app;
    app::g_app = &app;

    // main application window
    gui::mainwindow win(app);
    gui::g_win = &win;

    // accounts module
    app::accounts app_accounts;
    gui::accounts gui_accounts(app_accounts);
    app.attach(&app_accounts);
    win.attach(&gui_accounts);

    // groups module
    app::groups app_groups;
    gui::groups gui_groups(app_groups);
    app.attach(&app_groups);
    win.attach(&gui_groups);

    app::tasklist app_tasks;
    app::connlist app_conns;
    gui::downloads gui_downloads(app_tasks, app_conns);
    win.attach(&gui_downloads);

    // eventlog module
    app::eventlog& app_eventlog = app::eventlog::get();
    gui::eventlog  gui_eventlog(app_eventlog);
    app.attach(&app_eventlog);
    win.attach(&gui_eventlog);

    // RSS module
    app::rss app_rss(app);
    gui::rss gui_rss(win, app_rss);
    app.attach(&app_rss);
    win.attach(&gui_rss);

    // NZB module
    app::nzbfile app_nzb;
    gui::nzbfile gui_nzb(app_nzb);
    app.attach(&app_nzb);
    win.attach(&gui_nzb);

    app.loadstate();
    win.loadstate();

    win.show();



    return qtinstance.exec();
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