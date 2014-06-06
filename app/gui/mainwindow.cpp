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

#include <newsflash/sdk/format.h>

#include "mainwindow.h"
#include "config.h"
#include "../valuestore.h"
#include "../debug.h"

using sdk::str;

namespace gui
{

MainWindow::MainWindow() : QMainWindow(nullptr), current_(nullptr)
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

    DEBUG("MainWindow created");
}

MainWindow::~MainWindow()
{
    ui_.mainTab->clear();

    DEBUG("MainWindow deleted");
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
    values.set("window", "show_toolbar", ui_.mainToolBar->isVisible());
    values.set("window", "show_statusbar",ui_.statusBar->isVisible());
}

void MainWindow::attach(sdk::uicomponent* ui)
{
    Q_ASSERT(tabs_.indexOf(ui) == -1);

    const auto index = tabs_.size();
    const auto& text = ui->windowTitle();
    const auto& icon = ui->windowIcon();

    QAction* action = ui_.menuView->addAction(text);
    action->setCheckable(true);
    action->setChecked(false);
    action->setProperty("index", index);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(actionWindowToggle_triggered()));

    tabs_.append(ui);    
    tabs_actions_.append(action);
}

void MainWindow::detach(sdk::uicomponent* ui)
{
    const auto index = tabs_.indexOf(ui);
    Q_ASSERT(index != -1);

    hide(ui);

    tabs_.removeAt(index);
    tabs_actions_.removeAt(index);

    auto actions = ui_.menuView->actions();
    auto* action = actions[index];
    ui_.menuView->removeAction(action);
}

void MainWindow::show(sdk::uicomponent* ui)
{
    const int index = tabs_.indexOf(ui);
    Q_ASSERT(index != -1);

    if (ui_.mainTab->indexOf(ui) != -1)
        return;

    auto* action = tabs_actions_[index];
    action->setChecked(true);

    const auto& icon = ui->windowIcon();
    const auto& text = ui->windowTitle();
    ui_.mainTab->insertTab(index, ui, icon, text);    

    build_window_menu();
}

void MainWindow::hide(sdk::uicomponent* ui)
{
    const int index = tabs_.indexOf(ui);
    Q_ASSERT(index != -1);

    const int tab_index = ui_.mainTab->indexOf(ui);
    if (tab_index == -1)
        return;

    auto* action = tabs_actions_[index];
    action->setChecked(false);

    ui_.mainTab->removeTab(tab_index);

    build_window_menu();
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

bool MainWindow::is_shown(const sdk::uicomponent* ui) const
{
    const auto ret = ui_.mainTab->indexOf(const_cast<sdk::uicomponent*>(ui));
    if (ret == -1)
        return false;

    return true;
}


void MainWindow::closeEvent(QCloseEvent* event)
{
    // if (!app_.shutdown())
    // {
    //     QMessageBox msg(this);
    //     msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    //     msg.setIcon(QMessageBox::Critical);
    //     msg.setText(tr("Failed to save application state.\r\n"
    //        "No current session or settings will be saved.\r\n"
    //        "Are you sure you want to quit?"));
    //     if (msg.exec() == QMessageBox::No)
    //     {
    //         event->ignore();
    //         return;
    //     }
    // }
    // event->accept();
}

void MainWindow::build_window_menu()
{
    ui_.menuWindow->clear();

    const auto count = ui_.mainTab->count();
    const auto curr  = ui_.mainTab->currentIndex();
    for (int i=0; i<count; ++i)
    {
        const auto& text = ui_.mainTab->tabText(i);
        const auto& icon = ui_.mainTab->tabIcon(i);
        QAction* action  = ui_.menuWindow->addAction(icon, text);
        action->setCheckable(true);
        action->setChecked(i == curr);
        action->setProperty("tab-index", i);
        if (i < 9)
            action->setShortcut(QKeySequence(Qt::ALT | (Qt::Key_1 + i)));

        QObject::connect(action, SIGNAL(triggered()),
            this, SLOT(actionWindowFocus_triggered()));
    }
    // and this is in the window menu
    ui_.menuWindow->addSeparator();
    ui_.menuWindow->addAction(ui_.actionWindowClose);
    ui_.menuWindow->addAction(ui_.actionWindowNext);
    ui_.menuWindow->addAction(ui_.actionWindowPrev);    
}

void MainWindow::on_mainTab_currentChanged(int index)
{
    //DEBUG(str("Current tab _1", index));

    if (current_)
        current_->deactivate();

    ui_.mainToolBar->clear();
    ui_.menuEdit->clear();

    if (index != -1)
    {
        auto* ui = static_cast<sdk::uicomponent*>(
            ui_.mainTab->widget(index)); 

        ui->activate(ui_.mainTab);
        ui->add_actions(*ui_.mainToolBar);
        ui->add_actions(*ui_.menuEdit);

        auto title = ui->windowTitle();
        auto space = title.indexOf(" ");
        if (space != -1)
            title.resize(space);

        ui_.menuEdit->setTitle(title);
        current_ = ui;
    }

    // add the stuff that is always in the edit menu
    ui_.mainToolBar->addSeparator();
    ui_.mainToolBar->addAction(ui_.actionContextHelp);

    build_window_menu();
}

void MainWindow::on_mainTab_tabCloseRequested(int tab)
{
    auto* ui = static_cast<sdk::uicomponent*>(ui_.mainTab->widget(tab));

    hide(ui);
}

void MainWindow::on_actionContextHelp_triggered()
{
    if (!current_)
        return;

    //const auto& info = current_->get_info();
    //app_.open_help(info.helpurl);
}

void MainWindow::on_actionWindowClose_triggered()
{
    const auto tab = ui_.mainTab->currentIndex();
    if (tab == -1)
        return;

    auto* ui = static_cast<sdk::uicomponent*>(
        ui_.mainTab->widget(tab));

    hide(ui);
}

void MainWindow::on_actionWindowNext_triggered()
{
    const auto tab = ui_.mainTab->currentIndex();
    if (tab == -1)
        return;

    auto* ui = static_cast<sdk::uicomponent*>(
        ui_.mainTab->widget(tab));

    const auto size  = tabs_.size();
    const auto index = tabs_.indexOf(ui);
    auto* next = tabs_[(index + 1) % size];
    focus(next);
}

void MainWindow::on_actionWindowPrev_triggered()
{
    const auto tab = ui_.mainTab->currentIndex();
    if (tab == -1)
        return;

    auto* ui = static_cast<sdk::uicomponent*>(
        ui_.mainTab->widget(tab));

    const auto size  = tabs_.size();
    const auto index = tabs_.indexOf(ui);
    auto* prev = tabs_[((index  == 0) ? size - 1 : index - 1)];
    focus(prev);
}

void MainWindow::actionWindowToggle_triggered()
{
    const auto* action = static_cast<QAction*>(sender());
    const auto index   = action->property("index").toInt();
    const bool visible = action->isChecked();

    //DEBUG(str("Toggle tab _1", index));

    auto* tab = tabs_[index];

    if (visible)
    {
        show(tab);
        focus(tab);
    }
    else 
    {
        hide(tab);
    }
}

void MainWindow::actionWindowFocus_triggered()
{
    auto* action = static_cast<QAction*>(sender());
    const auto tab_index = action->property("tab-index").toInt();

    auto* ui = static_cast<sdk::uicomponent*>(
        ui_.mainTab->widget(tab_index));

    action->setChecked(true);

    focus(ui);
}

} // gui
