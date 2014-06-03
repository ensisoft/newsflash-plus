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

#include <algorithm>

#include "mainwindow.h"
#include "../eventlog.h"
#include "../format.h"
#include "../config.h"
#include "../valuestore.h"
#include "../mainapp.h"
#include "../debug.h"

using app::str;

namespace gui
{

MainWindow::MainWindow(app::mainapp& app) : QMainWindow(nullptr), app_(app), current_(nullptr)
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
{
    ui_.mainTab->clear();
}

void MainWindow::configure(const app::valuestore& values)
{
    const auto width  = values.get("window", "width", 1200);
    const auto height = values.get("window", "height", 800);
    DEBUG(str("Mainwindow dimensions _1 x _2", width, height));

    resize(width, height);

    const auto x = values.get("window", "x", 0);
    const auto y = values.get("window", "y", 0);
    DEBUG(str("Mainwindow position _1, _2", x, y));
    
    move(x, y);

    const auto show_statusbar = values.get("window", "show_statusbar", true);
    const auto show_toolbar = values.get("window", "show_toolbar", true);
    ui_.mainToolBar->setVisible(show_toolbar);
    ui_.statusBar->setVisible(show_statusbar);
    ui_.actionViewToolbar->setChecked(show_toolbar);
    ui_.actionViewStatusbar->setChecked(show_statusbar);
}

void MainWindow::persist(app::valuestore& values)
{
    values.set("window", "width", width());
    values.set("window", "height", height());
    values.set("window", "x", x());
    values.set("window", "y", y());
    values.set("window", "show_toolbar", 
        ui_.mainToolBar->isVisible());
    values.set("window", "show_statusbar",
        ui_.statusBar->isVisible());
}

void MainWindow::attach(sdk::uicomponent* ui)
{
    Q_ASSERT(std::find(std::begin(tabs_),
        std::end(tabs_), ui) == std::end(tabs_));

    const auto index = tabs_.size();
    tabs_.append(ui);

    const auto& text = ui->windowTitle();
    const auto& icon = ui->windowIcon();

    QAction* action = ui_.menuView->addAction(text);
    action->setCheckable(true);
    //action->setIcon(icon);
    action->setChecked(false);
    action->setProperty("tab-index", index);
    QObject::connect(action, SIGNAL(triggered()), 
        this, SLOT(show_tab()));
}

void MainWindow::detach(sdk::uicomponent* ui)
{
    Q_ASSERT(std::find(std::begin(tabs_),
        std::end(tabs_), ui) != std::end(tabs_));

    if (current_ == ui)
        current_ = nullptr;

    int index = tabs_.indexOf(ui);
    tabs_.removeAt(index);

    index = ui_.mainTab->indexOf(ui);
    if (index != -1)
    {
        ui_.mainTab->removeTab(index);
    }
}

void MainWindow::show(sdk::uicomponent* ui)
{
    Q_ASSERT(std::find(std::begin(tabs_),
        std::end(tabs_), ui) != std::end(tabs_));

    if (ui_.mainTab->indexOf(ui) == -1)
    {
        const int index = tabs_.indexOf(ui);
        const auto& icon = ui->windowIcon();
        const auto& text = ui->windowTitle();
        ui_.mainTab->insertTab(index, ui, icon, text);
    }
}

void MainWindow::hide(sdk::uicomponent* ui)
{
    Q_ASSERT(std::find(std::begin(tabs_),
        std::end(tabs_), ui) != std::end(tabs_));

    const auto index = ui_.mainTab->indexOf(ui);
    if (index == -1)
        return;

    ui_.mainTab->removeTab(index);
}

void MainWindow::focus(sdk::uicomponent* ui)
{
    Q_ASSERT(std::find(std::begin(tabs_),
        std::end(tabs_), ui) != std::end(tabs_));

    const auto index = ui_.mainTab->indexOf(ui);
    if (index == -1)
        return;

    ui_.mainTab->setCurrentIndex(index);
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

void MainWindow::on_mainTab_currentChanged(int index)
{
    DEBUG(str("Current tab _1", index));

    sdk::uicomponent* tab = nullptr;
    if (index != -1)
        tab = static_cast<sdk::uicomponent*>(ui_.mainTab->widget(index));

    ui_.mainToolBar->clear();
    ui_.menuEdit->clear();

    if (current_)
        current_->deactivate();

    if (tab)
    { 
        tab->activate(ui_.mainTab);
        tab->add_actions(*ui_.mainToolBar);
        tab->add_actions(*ui_.menuEdit);

        auto title = tab->windowTitle();
        auto space = title.indexOf(" ");
        if (space != -1)
            title.resize(space);

        ui_.menuEdit->setTitle(title);
        current_ = tab;
    }

    // add the stuff that is always in the edit menu
    ui_.mainToolBar->addSeparator();
    ui_.mainToolBar->addAction(ui_.actionContextHelp);
}

void MainWindow::on_mainTab_tabCloseRequested(int index)
{
    DEBUG(str("Close tab _1", index));
}

void MainWindow::on_actionContextHelp_triggered()
{
    if (!current_)
        return;

    const auto& info = current_->get_info();
    app_.open_help(info.helpurl);
}

void MainWindow::show_tab()
{
    const auto* action = static_cast<QAction*>(sender());

    const auto index = action->property("tab-index").toInt();

    auto* tab = tabs_[index];
    if (action->isChecked()) 
    {
        show(tab);
        focus(tab);
    }
    else 
    {
        hide(tab);
    }
}

} // gui
