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
#include <iostream>
#include <exception>

#include "qtsingleapplication/qtsingleapplication.h"
#include "mainwindow.h"
#include "minidump.h"
#include "config.h"
#include "../mainapp.h"

using sdk::str;

namespace {
    QString     g_executable_path;
    QStringList g_cmd_line;
} // namespace


namespace gui
{

QString get_installation_directory()
{
#if defined(WINDOWS_OS)
    QString s;
    // Using the first argument of the executable doesnt work with Unicode
    // paths, but the system mangles the path
    std::wstring buff;
    buff.resize(1024);

    DWORD ret = GetModuleFileName(NULL, &buff[0], buff.size());
    if (ret == 0)
        return "";
    
    s = widen(buff);
    int i = s.lastIndexOf("\\");
    if (i ==-1)
        return "";
    s = s.left(i+1);
    return s;

#elif defined(LINUX_OS)

    return g_executable_path;

#endif

}

void set_cmd_line(int argc, char* argv[])
{
#if defined(WINDOWS_OS)
    int num_cmd_args = 0;
    LPWSTR* cmds = CommandLineToArgW(GetCommandLineW(),
        &num_cmd_args);

    for (int i=0; i<num_cmd_args)
    {
        g_cmd_line.push_back(widen(cmds[i]));
    }
    LocalFree(cmds);

#elif defined(LINUX_OS)
    std::string tmp;
    auto e = argv[0];
    auto i = std::strlen(argv[0]);
    for (i=i-1; i>0; --i)
    {
        if (e[i] == '/')
        {
            tmp.append(e, i+1);
            break;
        }
    }
    g_executable_path = sdk::widen(tmp.c_str());

    for (int i=0; i<argc; ++i)
    {
        g_cmd_line.push_back(sdk::widen(argv[0]));
    }

#endif
}

QStringList get_cmd_line()
{
    return g_cmd_line;
}

void openurl(const QString& url)
{
    QDesktopServices::openUrl(url);
}

void openhelp(const QString& help)
{
    const QString& home = get_installation_directory();
    const QString& file = QString("file://%1help/%2").arg(home).arg(help);
    QDesktopServices::openUrl(file);
}



// note that any Qt style must be applied first
// thing before any Qt application object is instance is created.
void set_style()
{
    const auto& home = QDir::homePath();
    const auto& file = home + "/.newsflash/style";
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

int run(int argc, char* argv[])
{
    set_cmd_line(argc, argv);
    set_style();

    const auto& home = QDir::homePath();

    QDir dir;
    if (!dir.mkpath(home + "/.newsflash"))
        throw std::runtime_error("Failed to create .newsflash in user home");

    QtSingleApplication qtinstance(argc, argv);
    if (qtinstance.isRunning())
    {
        const QStringList& cmds = get_cmd_line();
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

    const auto& self = gui::get_installation_directory();

    QCoreApplication::setLibraryPaths(QStringList());
    QCoreApplication::addLibraryPath(self);
    QCoreApplication::addLibraryPath(self + "/plugins-qt");

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