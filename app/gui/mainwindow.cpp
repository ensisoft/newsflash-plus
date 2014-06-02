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
#  include <QtGui/QCloseEvent>
#  include <QtGui/QMessageBox>
#  include <QEvent>
#  include <QTimer>
#include <newsflash/warnpop.h>

#include "mainwindow.h"
#include "../eventlog.h"
#include "../format.h"
#include "../config.h"
#include "../settings.h"
#include "../mainapp.h"

using app::str;

namespace gui
{

MainWindow::MainWindow(app::mainapp& app) : QMainWindow(nullptr), app_(app)
{
    TinyGraph::colors greenish = {};
    greenish.fill    = QColor(47, 117, 29, 150);
    greenish.grad1   = QColor(232, 232, 232);
    greenish.grad2   = QColor(200, 200, 200);
    greenish.outline = QColor(97, 212, 55);

    ui_.setupUi(this);

    // set the color scheme for the network monitor
    ui_.netGraph->set_colors(greenish);

    // the designer wont let remove the single tab    
    ui_.mainTab->removeTab(0); 

    // put the various little widgets in their correct places
    ui_.statusBar->insertPermanentWidget(0, ui_.frmProgress);
    ui_.statusBar->insertPermanentWidget(1, ui_.frmFreeSpace);
    ui_.statusBar->insertPermanentWidget(2, ui_.frmDiskWrite);
    ui_.statusBar->insertPermanentWidget(3, ui_.frmGraph);
    ui_.statusBar->insertPermanentWidget(4, ui_.frmKbs);    

    setWindowTitle(NEWSFLASH_TITLE);

}

MainWindow::~MainWindow()
{}

void MainWindow::apply(const app::settings& settings)
{
    const auto width  = settings.get("window", "width", 1200);
    const auto height = settings.get("window", "height", 800);
    DEBUG(str("Mainwindow dimensions _1 x _2", 
        width, height));

    resize(width, height);

    const auto x = settings.get("window", "x", 0);
    const auto y = settings.get("window", "y", 0);
    DEBUG(str("Mainwindow position _1, _2",
        x, y));

    move(x, y);

    const auto show_statusbar = settings.get("window", "show_statusbar", true);
    const auto show_toolbar = settings.get("window", "show_toolbar", true);
    ui_.mainToolBar->setVisible(show_toolbar);
    ui_.statusBar->setVisible(show_statusbar);
    ui_.actionViewToolbar->setChecked(show_toolbar);
    ui_.actionViewStatusbar->setChecked(show_statusbar);
}

void MainWindow::persist(app::settings& settings)
{
    settings.set("window", "width", width());
    settings.set("window", "height", height());
    settings.set("window", "x", x());
    settings.set("window", "y", y());
    settings.set("window", "show_statusbar", 
        ui_.mainToolBar->isVisible());
    settings.set("window", "show_statusbar",
        ui_.statusBar->isVisible());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!app_.shutdown())
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setIcon(QMessageBox::Critical);
        msg.setText(tr("Failed to save application state.\r\n"
           "No current session or settings will be saved.\r\n"
           "Are you sure you want to quit?"));
        if (msg.exec() == QMessageBox::No)
        {
            event->ignore();
            return;
        }
    }
    event->accept();
}

} // gui
