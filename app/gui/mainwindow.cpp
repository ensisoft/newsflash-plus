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
#  include <QtGui/QCloseEvent>
#  include <QtGui/QMessageBox>
#  include <QtGui/QSystemTrayIcon>
#  include <QEvent>
#  include <QTimer>
#  include <QDir>
#  include <QLibrary>
#include <newsflash/warnpop.h>

#include <newsflash/sdk/widget.h>
#include <newsflash/sdk/format.h>
#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/home.h>
#include <newsflash/sdk/dist.h>
#include <newsflash/sdk/message.h>

#include "mainwindow.h"
#include "accounts.h"
#include "eventlog.h"
#include "groups.h"
#include "dlgwelcome.h"
#include "config.h"
#include "message.h"
#include "../mainapp.h"



using sdk::str;

namespace gui
{

void openurl(const QString& url);
void openhelp(const QString& page);


MainWindow::MainWindow(app::mainapp& app) : QMainWindow(nullptr), 
    app_(app), current_(nullptr)
{
    ui_.setupUi(this);

    // the designer wont let remove the single tab.
    // NOTE: this will call signal handlers for the tab, currentChanged etc.
    // its best to do this first.
    ui_.mainTab->removeTab(0); 

    // set network monitor colors
    TinyGraph::colors greenish = {};
    greenish.fill    = QColor(47, 117, 29, 150);
    greenish.grad1   = QColor(232, 232, 232);
    greenish.grad2   = QColor(200, 200, 200);
    greenish.outline = QColor(97, 212, 55);
    ui_.netGraph->set_colors(greenish);

    // put the various little widgets in their correct places
    ui_.statusBar->insertPermanentWidget(0, ui_.frmProgress);
    ui_.statusBar->insertPermanentWidget(1, ui_.frmFreeSpace);
    ui_.statusBar->insertPermanentWidget(2, ui_.frmDiskWrite);
    ui_.statusBar->insertPermanentWidget(3, ui_.frmGraph);
    ui_.statusBar->insertPermanentWidget(4, ui_.frmKbs);    

    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        WARN("System tray is not available. Notifications disabled");
    }
    else
    {
        DEBUG("Setup system tray");

        ui_.actionRestore->setEnabled(false);
        ui_.actionMinimize->setEnabled(true);

        tray_.setIcon(QIcon(":/resource/16x16_ico_png/ico_newsflash.png"));
        tray_.setToolTip(NEWSFLASH_TITLE " " NEWSFLASH_VERSION);
        tray_.setVisible(true);

        QObject::connect(&tray_, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(actionTray_activated(QSystemTrayIcon::ActivationReason)));
    }

    setWindowTitle(NEWSFLASH_TITLE);

    loadstate();
    loadwidgets();

    DEBUG("MainWindow created");
}

MainWindow::~MainWindow()
{
    ui_.mainTab->clear();

    // todo: use unique_ptr
    for (auto* tab : tabs_)
    {
        delete tab;
    }
    
    DEBUG("MainWindow deleted");
}

sdk::model* MainWindow::create_model(const char* klazz)
{
    return app_.create_model(klazz);
}

void MainWindow::show(const QString& tabname)
{
    for (auto* tab : tabs_)
    {
        if (tab->windowTitle() == tabname)
        {
            show(tab);
            break;
        }
    }
}

void MainWindow::show(sdk::widget* widget)
{
    const int index = tabs_.indexOf(widget);
    Q_ASSERT(index != -1);

    if (ui_.mainTab->indexOf(widget) != -1)
        return;

    auto* action = tabs_actions_[index];
    action->setChecked(true);

    const auto& icon = widget->windowIcon();
    const auto& text = widget->windowTitle();
    ui_.mainTab->insertTab(index, widget, icon, text);    

    build_window_menu();
}

void MainWindow::hide(sdk::widget* ui)
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

void MainWindow::focus(sdk::widget* ui)
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
    if (!app_.savestate() || !savestate())
    {
        if (QMainWindow::isHidden())
        {
            on_actionRestore_triggered();
        }

        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setIcon(QMessageBox::Critical);
        msg.setText(tr("Failed to save application state.\r\n"
           "No current session or settings will be saved.\r\n"
           "Are you sure you want to quit?"));
        if (msg.exec() == QMessageBox::No)
        {
            show("Logs");
            event->ignore();
            return;
        }
    }

    event->accept();

    ui_.mainTab->clear();
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

bool MainWindow::savestate()
{
    settings_.set("window", "width", width());
    settings_.set("window", "height", height());
    settings_.set("window", "x", x());
    settings_.set("window", "y", y());
    settings_.set("window", "show_toolbar", ui_.mainToolBar->isVisible());
    settings_.set("window", "show_statusbar", ui_.statusBar->isVisible()); 

    for (std::size_t i=0; i<tabs_.size(); ++i)
    {
        const auto& text = tabs_[i]->windowTitle();
        const auto show  = tabs_actions_[i]->isChecked();
        settings_.set("visible_tabs", text, show);

        tabs_[i]->save(settings_);
    }

    const auto file  = sdk::home::file("gui.json");
    const auto error = settings_.save(file);
    if (error != QFile::NoError)
        ERROR(str("Failed to save settings _1, _2", error, file));

    return error == QFile::NoError;
}

void MainWindow::loadstate()
{
    // load gui settings, we need this for the rest of the stuff
    const auto file = sdk::home::file("gui.json");
    if (!QFile::exists(file))
    {
        // assuming first launch if settings file doesn't exist
        QTimer::singleShot(500, this, SLOT(timerWelcome_timeout()));        
        return;
    }

    const auto error = settings_.load(file);
    if (error != QFile::NoError)
    {
        ERROR(str("Failed to load settings _1, _2", error, file));
        return;
    }

    const auto width  = settings_.get("window", "width", 1200);
    const auto height = settings_.get("window", "height", 800);
    DEBUG(str("Mainwindow dimensions _1 x _2", width, height));
    resize(width, height);

    const auto x = settings_.get("window", "x", 0);
    const auto y = settings_.get("window", "y", 0);
    DEBUG(str("Mainwindow position _1, _2", x, y));
    move(x, y);

    const auto show_statusbar = settings_.get("window", "show_statusbar", true);
    const auto show_toolbar = settings_.get("window", "show_toolbar", true);
    ui_.mainToolBar->setVisible(show_toolbar);
    ui_.statusBar->setVisible(show_statusbar);
    ui_.actionViewToolbar->setChecked(show_toolbar);
    ui_.actionViewStatusbar->setChecked(show_statusbar);


}

void MainWindow::loadwidgets()
{
    tabs_.append(new Accounts(app_.get_model("accounts")));
    tabs_.append(new Groups(app_.get_model("groups")));
    tabs_.append(new Eventlog(app_.get_model("eventlog")));


    const QDir dir(sdk::dist::path("plugins"));

#define FAIL(x) \
    if (1) { \
        ERROR(str("Failed to load plugin library _1, _2", lib, x)); \
        continue; \
    }

    for (const auto& name : dir.entryList())
    {
        const auto& file = dir.absoluteFilePath(name);
        if (!QLibrary::isLibrary(file))
            continue;

        QLibrary lib(file);

        DEBUG(str("Loading plugin _1", lib));

        if (!lib.load())
        {
            ERROR(lib.errorString());
            continue;
        }
        auto get_api_version = (sdk::fp_widget_api_version)(lib.resolve("widget_api_version"));
        if (get_api_version == nullptr)
            FAIL("no api version found");

        if (get_api_version() != sdk::widget::version)
            FAIL("incompatible version");

        auto create = (sdk::fp_widget_create)(lib.resolve("create_widget"));
        if (create == nullptr)
            FAIL("no entry point found");

        sdk::widget* widget = create(this);
        if (widget == nullptr)
            FAIL("widget create failed");

        tabs_.append(widget);

        //DEBUG(str("Created instance of _1"))
    }

#undef FAIL

    for (int i=0; i<tabs_.size(); ++i)
    {
        const auto& text = tabs_[i]->windowTitle();
        const auto& icon = tabs_[i]->windowIcon();
        const auto& info = tabs_[i]->information();
        const auto show  = settings_.get("visible_tabs", text, info.visible_by_default);

        QAction* action = ui_.menuView->addAction(text);
        action->setCheckable(true);
        action->setChecked(show);
        action->setProperty("index", i);
        QObject::connect(action, SIGNAL(triggered()), this, SLOT(actionWindowToggle_triggered()));        

        tabs_actions_.append(action);        
        if (show)
            ui_.mainTab->insertTab(i, tabs_[i], icon, text);

        tabs_[i]->load(settings_);
    }    
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
        auto* widget = static_cast<sdk::widget*>(
            ui_.mainTab->widget(index)); 

        widget->activate(ui_.mainTab);
        widget->add_actions(*ui_.mainToolBar);
        widget->add_actions(*ui_.menuEdit);

        auto title = widget->windowTitle();
        auto space = title.indexOf(" ");
        if (space != -1)
            title.resize(space);

        ui_.menuEdit->setTitle(title);
        current_ = widget;
    }

    // add the stuff that is always in the edit menu
    ui_.mainToolBar->addSeparator();
    ui_.mainToolBar->addAction(ui_.actionContextHelp);

    // build file menu
    ui_.menuFile->clear();
    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        ui_.menuFile->addAction(ui_.actionMinimize);
    }
    ui_.menuFile->addAction(ui_.actionExit);

    build_window_menu();
}

void MainWindow::on_mainTab_tabCloseRequested(int tab)
{
    auto* widget = static_cast<sdk::widget*>(ui_.mainTab->widget(tab));

    hide(widget);
}

void MainWindow::on_actionWindowClose_triggered()
{
    const auto tab = ui_.mainTab->currentIndex();
    if (tab == -1)
        return;

    auto* widget = static_cast<sdk::widget*>(
        ui_.mainTab->widget(tab));

    hide(widget);
}

void MainWindow::on_actionWindowNext_triggered()
{
    const auto tab = ui_.mainTab->currentIndex();
    if (tab == -1)
        return;

    auto* widget = static_cast<sdk::widget*>(
        ui_.mainTab->widget(tab));

    const auto size  = tabs_.size();
    const auto index = tabs_.indexOf(widget);
    auto* next = tabs_[(index + 1) % size];
    focus(next);
}

void MainWindow::on_actionWindowPrev_triggered()
{
    const auto tab = ui_.mainTab->currentIndex();
    if (tab == -1)
        return;

    auto* widget = static_cast<sdk::widget*>(
        ui_.mainTab->widget(tab));

    const auto size  = tabs_.size();
    const auto index = tabs_.indexOf(widget);
    auto* prev = tabs_[((index  == 0) ? size - 1 : index - 1)];
    focus(prev);
}

void MainWindow::on_actionHelp_triggered()
{
    openhelp("index.html");
}

void MainWindow::on_actionMinimize_triggered()
{
    DEBUG("Minimize to tray!");

    QMainWindow::hide();

    ui_.actionRestore->setEnabled(true);
    ui_.actionMinimize->setEnabled(false);
}

void MainWindow::on_actionRestore_triggered()
{
    DEBUG("Restore from tray");

    QMainWindow::show();

    ui_.actionRestore->setEnabled(false);
    ui_.actionMinimize->setEnabled(true);
}

void MainWindow::on_actionContextHelp_triggered()
{
    const auto* widget = static_cast<sdk::widget*>(
        ui_.mainTab->currentWidget());
    if (widget == nullptr)
        return;

    const auto& info = widget->information();

    openhelp(info.helpurl);
}

void MainWindow::on_actionExit_triggered()
{
    // calling close will generate QCloseEvent which
    // will invoke the normal shutdown sequence.
    QMainWindow::close();
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

    auto* widget = static_cast<sdk::widget*>(
        ui_.mainTab->widget(tab_index));

    action->setChecked(true);

    focus(widget);
}




void MainWindow::actionTray_activated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Context)
        return;

    QMenu menu;
    menu.addAction(ui_.actionRestore);
    menu.addSeparator();
    menu.addAction(ui_.actionExit);
    menu.exec(QCursor::pos());
}

void MainWindow::timerWelcome_timeout()
{
    DlgWelcome dlg(this);
    dlg.exec();
    if (dlg.open_guide())
        openhelp("quick.html");

    const auto add_account = dlg.add_account();

    sdk::send(gui::msg_first_launch {add_account}, "gui");

}

} // gui
