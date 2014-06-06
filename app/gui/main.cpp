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
#  include <QtGui/QStyle>
#  include <QtGui/QMessageBox>
#  include <QDir>
#  include <QFile>
#include <newsflash/warnpop.h>

#include <newsflash/sdk/format.h>

#include <iostream>
#include <exception>

#include "qtsingleapplication/qtsingleapplication.h"

#include "mainwindow.h"
#include "minidump.h"
#include "config.h"
#include "cmdline.h"
#include "../mainapp.h"
#include "../eventlog.h"
#include "../debug.h"
#include "../valuestore.h"

int run(int argc, char* argv[])
{
    const auto& home = QDir::homePath();
    const auto& file = "./newsflash/gui.json";

    gui::set_cmd_line(argc, argv);

    app::valuestore values;
    if (QFile::exists(file))
    {
        QFile io(file);
        if (!io.open(QIODevice::ReadWrite))
        {
            ERROR(sdk::str("Failed to open _1", io));
        }
        values.load(io);
    }

    // Qt style must be applied first thing before any Qt application object is created.
    const auto& name = values.get("gui", "style", "default");
    if (name != "default")
    {
        QStyle* style = QApplication::setStyle(name);
        if (style)
            QApplication::setPalette(
                style->standardPalette());
    }

    QtSingleApplication qtinstance(argc, argv);
    if (qtinstance.isRunning())
    {
        const QStringList& cmds = gui::get_cmd_line();
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

    app::mainapp app;

    // bring up the main window
    gui::MainWindow window;


    window.configure(values);
    window.show();

    qtinstance.exec();

    return 0;
}

int main(int argc, char* argv[])
{
    DEBUG("It's alive!");
    DEBUG(NEWSFLASH_TITLE << NEWSFLASH_VERSION);
    try 
    {
        return SEH_BLOCK(run(argc, argv));
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        std::cerr << std::endl;
        //QMessageBox
    }
    return 1;
}