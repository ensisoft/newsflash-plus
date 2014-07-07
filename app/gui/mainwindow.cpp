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
#  include <QtGui/QFileDialog>
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
#include "dlgsettings.h"
#include "config.h"
#include "message.h"
#include "engine_settings.h"
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
    
    DEBUG("MainWindow deleted");
}

sdk::model* MainWindow::create_model(const char* klazz)
{
    return app_.create_model(klazz);
}

void MainWindow::show_widget(const QString& name)
{
    show(name);
}

void MainWindow::show_setting(const QString& name)
{
    DlgSettings dlg(this);

    app::settings settings;
    app_.get(settings);

    std::vector<std::unique_ptr<sdk::settings>> pages;
    pages.emplace_back(new engine_settings(settings));
    
    for (auto& widget : widgets_)
    {
        widget->add_settings(pages);
    }
    for (auto& page : pages)
    {
        dlg.attach(page.get());
    }

    if (!name.isEmpty())
        dlg.show(name);

    if (dlg.exec() == QDialog::Accepted)
    {
        app_.set(settings);
    }    
}

QString MainWindow::select_download_folder()
{
    QString latest;
    if (!recents_.isEmpty())
        latest = recents_.back();

    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setWindowTitle(tr("Select download folder"));
    dlg.setDirectory(latest);
    if (dlg.exec() == QDialog::Rejected)
        return QString();

    auto dir = dlg.selectedFiles().first();

    Q_ASSERT(dir.isEmpty());

    if (!recents_.contains(dir))
        recents_.push_back(dir);

    if (recents_.size() > 10)
        recents_.pop_front();

    return QDir::toNativeSeparators(dir);
}

void MainWindow::recents(QStringList& paths) const 
{
    for (const auto& path : recents_)
    {
        QDir dir(path);
        const auto& clean    = QDir::cleanPath(dir.absolutePath());
        const auto& native   = QDir::toNativeSeparators(clean);
        paths << native;
    }
}

void MainWindow::show(const QString& title)
{
    const auto it = std::find_if(std::begin(widgets_), std::end(widgets_),
        [&](const std::unique_ptr<sdk::widget>& widget) {
            return widget->windowTitle() == title;
        });
    if (it == std::end(widgets_))
        return;

    show(std::distance(std::begin(widgets_), it));
}

void MainWindow::show(sdk::widget* widget)
{
    auto it = std::find_if(std::begin(widgets_), std::end(widgets_),
        [&](const std::unique_ptr<sdk::widget>& maybe) {
            return maybe.get()  == widget;
        });
    if (it == std::end(widgets_))
        return;

    show(std::distance(std::begin(widgets_), it));
}

void MainWindow::show(std::size_t index)
{
    auto& widget = widgets_[index];
    auto* action = actions_[index];

    // if already show, then do nothing
    auto pos = ui_.mainTab->indexOf(widget.get());
    if (pos != -1)
        return;

    action->setChecked(true);

    const auto& icon = widget->windowIcon();
    const auto& text = widget->windowTitle();
    ui_.mainTab->insertTab(index, widget.get(), icon, text);
    build_window_menu();

}

void MainWindow::hide(sdk::widget* widget)
{
    auto it = std::find_if(std::begin(widgets_), std::end(widgets_),
        [&](const std::unique_ptr<sdk::widget>& maybe) {
            return maybe.get() == widget;
        });
    Q_ASSERT(it != std::end(widgets_));

    auto index = std::distance(std::begin(widgets_), it);

    hide(index);

}

void MainWindow::hide(std::size_t index)
{
    auto& widget = widgets_[index];
    auto* action = actions_[index];

    // if already not shown then do nothing
    auto pos = ui_.mainTab->indexOf(widget.get());
    if (pos == -1)
        return;

    action->setChecked(false);

    ui_.mainTab->removeTab(pos);
 
    build_window_menu();
}

void MainWindow::focus(sdk::widget* ui)
{
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
    settings_.set("settings", "recent_paths", recents_);

    for (std::size_t i=0; i<widgets_.size(); ++i)
    {
        const auto& text = widgets_[i]->windowTitle();
        const auto show  = actions_[i]->isChecked();
        settings_.set("visible_tabs", text, show);

        widgets_[i]->save(settings_);
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

    recents_ = settings_.get("settings", "recent_paths").toStringList();

}

void MainWindow::loadwidgets()
{
    widgets_.emplace_back(new Accounts(app_.get_model("accounts")));
    widgets_.emplace_back(new Groups(app_.get_model("groups")));
    widgets_.emplace_back(new Eventlog(app_.get_model("eventlog")));

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

        auto create = (sdk::fp_widget_create)(lib.resolve("create_widget"));
        if (create == nullptr)
            FAIL("no entry point found");

        std::unique_ptr<sdk::widget> widget(create(this, sdk::widget::version));
        if (!widget)
            continue;

        widgets_.push_back(std::move(widget));

        INFO(str("Loaded _1", lib));
    }

#undef FAIL

    for (std::size_t i=0; i<widgets_.size(); ++i)
    {
        const auto& text = widgets_[i]->windowTitle();
        const auto& icon = widgets_[i]->windowIcon();
        const auto& info = widgets_[i]->information();
        const auto show  = settings_.get("visible_tabs", text, info.visible_by_default);

        QAction* action = ui_.menuView->addAction(text);
        action->setCheckable(true);
        action->setChecked(show);
        action->setProperty("index", (int)i);
        QObject::connect(action, SIGNAL(triggered()), this, SLOT(actionWindowToggleView_triggered()));        

        actions_.push_back(action);        
        if (show)
            ui_.mainTab->insertTab(i, widgets_[i].get(), icon, text);

        widgets_[i]->load(settings_);
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
    const auto* widget = static_cast<sdk::widget*>(ui_.mainTab->widget(tab));

    // find in the array
    auto it = std::find_if(std::begin(widgets_), std::end(widgets_), 
        [&](const std::unique_ptr<sdk::widget>& maybe) {
            return maybe.get() == widget;
        });
    Q_ASSERT(it != std::end(widgets_));

    auto index = std::distance(std::begin(widgets_), it);

    auto* action = actions_[index];
    action->setChecked(false);

    ui_.mainTab->removeTab(tab);

    build_window_menu();
}

void MainWindow::on_actionWindowClose_triggered()
{
    const auto cur = ui_.mainTab->currentIndex();
    if (cur == -1)
        return;

    on_mainTab_tabCloseRequested(cur);
}

void MainWindow::on_actionWindowNext_triggered()
{
    // cycle to next tab in the maintab

    const auto cur = ui_.mainTab->currentIndex();
    if (cur == -1)
        return;

    const auto size = ui_.mainTab->count();
    const auto next = (cur + 1) % size;
    ui_.mainTab->setCurrentIndex(next);
}

void MainWindow::on_actionWindowPrev_triggered()
{
    // cycle to previous tab in the maintab

    const auto cur = ui_.mainTab->currentIndex();
    if (cur == -1)
        return;

    const auto size = ui_.mainTab->count();
    const auto prev = (cur == 0) ? size - 1 : cur - 1;
    ui_.mainTab->setCurrentIndex(prev);
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
    const auto* widget = static_cast<sdk::widget*>(ui_.mainTab->currentWidget());
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

void MainWindow::on_actionSettings_triggered()
{
    show_setting("");


}

void MainWindow::actionWindowToggleView_triggered()
{
    // the signal comes from the action object in
    // the view menu. the index is the index to the widgets array

    const auto* action = static_cast<QAction*>(sender());
    const auto index   = action->property("index").toInt();
    const bool visible = action->isChecked();

    auto& widget = widgets_[index];

    if (visible)
    {
        show(index);
//        focus(index);
    }
    else 
    {
        hide(index);
    }
}

void MainWindow::actionWindowFocus_triggered()
{
    // this signal comes from an action in the
    // window menu. the index is the index of the widget
    // in the main tab. the menu is rebuilt
    // when the maintab configuration changes.

    auto* action = static_cast<QAction*>(sender());
    const auto tab_index = action->property("tab-index").toInt();

    action->setChecked(true);

    ui_.mainTab->setCurrentIndex(tab_index);
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
