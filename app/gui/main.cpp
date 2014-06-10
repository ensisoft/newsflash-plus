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

#include <newsflash/sdk/format.h>
#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/home.h>
#include <newsflash/sdk/dist.h>
#include <iostream>
#include <exception>

#include <Python.h>

#include "qtsingleapplication/qtsingleapplication.h"
#include "mainwindow.h"
#include "minidump.h"
#include "config.h"
#include "../mainapp.h"

using sdk::str;

namespace gui
{

void openurl(const QString& url)
{
    QDesktopServices::openUrl(url);
}

void openhelp(const QString& page)
{
    const auto& help = sdk::dist::path("help");
    const auto& file = QString("file://%1/%2").arg(help).arg(page);
    QDesktopServices::openUrl(file);
}

// note that any Qt style must be applied first
// thing before any Qt application object is instance is created.
void set_style()
{
    const auto& file = sdk::home::file("style");
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
    INFO(str("Python _1", Py_GetVersion()));
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


    sdk::home::init(".newsflash");
    sdk::dist::init();

    set_style();

    QCoreApplication::setLibraryPaths(QStringList());
    QCoreApplication::addLibraryPath(sdk::dist::path());
    QCoreApplication::addLibraryPath(sdk::dist::path("plugins-qt"));

    copyright();

    // main application module
    app::mainapp app(qtinstance);

    // bring up the main window
    gui::MainWindow window(app);

    window.show();

    return qtinstance.exec();
}

} // gui

int main(int argc, char* argv[])
{
    DEBUG("It's alive!");
    DEBUG(NEWSFLASH_TITLE << NEWSFLASH_VERSION);
    try 
    {
        return SEH_BLOCK(gui::run(argc, argv));
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        std::cerr << std::endl;

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