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

#include <newsflash/warnpush.h>
#  include <QtGui/QApplication>
#  include <QtGui/QStyle>
#  include <QCoreApplication>
#  include <QStringList>
#  include <QTextStream>
#  include <QString>
#  include <QFile>
#  include <QDir>
#  include <newsflash/warnpop.h>

#include <cassert>
#include <iostream>
#include <exception>

#include "qtsingleapplication/qtsingleapplication.h"
#include "mainwindow.h"
#include "minidump.h"
#include "utility.h"

namespace gui
{

int main(int argc, char* argv[])
{
    app::set_cmd_line(argc, argv);

    const auto path = app::get_installation_directory();

    QCoreApplication::setLibraryPaths(QStringList());
    QCoreApplication::addLibraryPath(path);
    QCoreApplication::addLibraryPath(path + "/plugins-qt");

    try
    {
        QtSingleApplication newsflash(argc, argv);
        if (newsflash.isRunning())
        {
            const QStringList& cmds = app::get_cmd_line();
            for (int i=1; i<cmds.size(); ++i)
                newsflash.sendMessage(cmds[i]);
            return 0;            
        }

        const auto home = QDir::homePath();
        const auto file = home + "/.newsflash/style";
        QFile style(file);
        if (style.open(QIODevice::ReadOnly))
        {
            QTextStream stream(&style);
            QString name = stream.readLine();
            if (!name.isEmpty() && 
                 name.toLower() != "default")
            {
                QStyle* theme = QApplication::setStyle(name);
                if (theme)
                    QApplication::setPalette(theme->standardPalette());
            }
        }

        gui::MainWindow window;
        window.show();

        return newsflash.exec();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
    }
    return 0;
}

} // gui

int main(int argc, char* argv[])
{
    SEH_BLOCK
    (
        return gui::main(argc, argv);
    );
    return 0;
}