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

#define LOGTAG "mainwindow"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <boost/version.hpp>
#  include <QtGui/QCloseEvent>
#  include <QtGui/QMessageBox>
#  include <QtGui/QSystemTrayIcon>
#  include <QtGui/QFileDialog>
#  include <QtGui/QDesktopWidget>
#  include <QEvent>
#  include <QTimer>
#  include <QDir>
#  include <QFileInfo>
#include <newsflash/warnpop.h>

#include "mainwindow.h"
#include "mainwidget.h"
#include "mainmodule.h"
#include "settings.h"
#include "dlgwelcome.h"
#include "dlgsettings.h"
#include "dlgaccount.h"
#include "dlgchoose.h"
#include "dlgshutdown.h"
#include "dlgabout.h"
#include "config.h"
#include "../eventlog.h"
#include "../debug.h"
#include "../format.h"
#include "../homedir.h"
#include "../settings.h"
#include "../accounts.h"
#include "../engine.h"

using app::str;

namespace gui
{

mainwindow* g_win;    

void openurl(const QString& url);
void openhelp(const QString& page);


mainwindow::mainwindow(app::settings& s) : QMainWindow(nullptr), current_(nullptr), settings_(s)
{
    ui_.setupUi(this);

    // set network monitor colors
    TinyGraph::colors greenish = {};
    greenish.fill    = QColor(47, 117, 29, 150);
    greenish.grad1   = QColor(232, 232, 232);
    greenish.grad2   = QColor(200, 200, 200);
    greenish.outline = QColor(97, 212, 55);
    ui_.netGraph->setColors(greenish);

    // put the various little widgets in their correct places
    ui_.statusBar->insertPermanentWidget(0, ui_.frmProgress);
    ui_.statusBar->insertPermanentWidget(1, ui_.frmFreeSpace);
    ui_.statusBar->insertPermanentWidget(2, ui_.frmDiskWrite);
    ui_.statusBar->insertPermanentWidget(3, ui_.frmGraph);
    ui_.statusBar->insertPermanentWidget(4, ui_.frmKbs);    

    if (QSystemTrayIcon::isSystemTrayAvailable())
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

    ui_.mainToolBar->setVisible(true);
    ui_.statusBar->setVisible(true);
    ui_.actionViewToolbar->setChecked(true);
    ui_.actionViewStatusbar->setChecked(true);

    DEBUG("mainwindow created");

    QObject::connect(&refresh_timer_, SIGNAL(timeout()),
        this, SLOT(timerRefresh_timeout()));
    refresh_timer_.setInterval(1000);
    refresh_timer_.start();
}

mainwindow::~mainwindow()
{
    DEBUG("mainwindow deleted");
}

void mainwindow::attach(mainwidget* widget)
{
    Q_ASSERT(!widget->parent());

    int index = widgets_.size();

    widgets_.push_back(widget);

    const auto& text = widget->windowTitle();
    const auto& icon = widget->windowIcon();
    const auto& info = widget->information();

    // only permanent widgets get a view action.
    // permant widgets are those that instead of being closed
    // are just hidden and toggled in the View menu. 
    if (info.permanent)
    {
        QAction* action = ui_.menuView->addAction(text);
        action->setCheckable(true);
        action->setChecked(true);
        action->setProperty("index", index);
        QObject::connect(action, SIGNAL(triggered()), this, 
            SLOT(actionWindowToggleView_triggered()));

        actions_.push_back(action);
    }
    else
    {
        const auto count = ui_.mainTab->count();
        ui_.mainTab->addTab(widget, icon, text);
        ui_.mainTab->setCurrentIndex(count);
    }

    build_window_menu();
}

void mainwindow::attach(mainmodule* module)
{
    modules_.push_back(module);
}

void mainwindow::detach_all_widgets()
{
    ui_.mainTab->clear();

    for (auto* w : widgets_)
        w->setParent(nullptr);

    widgets_.clear();
}

void mainwindow::loadstate()
{
    // load global objects state.
    app::g_accounts->loadstate(settings_);
    app::g_engine->loadstate(settings_);

    if (!settings_.contains("window", "width"))
    {
        // if the settings file doesn't exist then we assume that this is the
        // first launch of the application and just setup for the user welcome
        QTimer::singleShot(500, this, SLOT(timerWelcome_timeout()));
    }

    const auto width  = settings_.get("window", "width", 0);
    const auto height = settings_.get("window", "height", 0);
    if (width && height)
    {
        DEBUG(str("mainwindow dimensions _1 x _2", width, height));        
        resize(width, height);
    }

    const auto x = settings_.get("window", "x", 0);
    const auto y = settings_.get("window", "y", 0);

    QDesktopWidget desktop;
    auto screen = desktop.availableGeometry(desktop.primaryScreen());
    if (x < screen.width() - 50 && y < screen.height() - 50)
    {
        DEBUG(str("mainwindow position _1, _2", x, y));
        move(x, y);
    }

    const auto show_statbar = settings_.get("window", "show_statusbar", true);
    const auto show_toolbar = settings_.get("window", "show_toolbar", true);
    ui_.mainToolBar->setVisible(show_toolbar);
    ui_.statusBar->setVisible(show_statbar);
    ui_.actionViewToolbar->setChecked(show_toolbar);
    ui_.actionViewStatusbar->setChecked(show_statbar);

    recents_ = settings_.get("paths", "recents").toStringList();
    recent_save_nzb_path_ = settings_.get("paths", "save_nzb").toString();
    recent_load_nzb_path_ = settings_.get("paths", "load_nzb").toString();

    for (std::size_t i=0; i<widgets_.size(); ++i)
    {
        const auto text = widgets_[i]->windowTitle();
        const auto icon = widgets_[i]->windowIcon();
        const auto info = widgets_[i]->information();
        const auto show = settings_.get("window_visible_tabs", text, info.visible_by_default);
        if (show)
        {
            ui_.mainTab->insertTab(i, widgets_[i], icon, text);
        }
        if (i < actions_.size())
            actions_[i]->setChecked(show);        
          
        widgets_[i]->loadstate(settings_);
    }

    for (auto* m : modules_)
        m->loadstate(settings_);
}

void mainwindow::show_widget(const QString& name)
{
    show(name);
}

void mainwindow::show_setting(const QString& name)
{
    DlgSettings dlg(this);

    std::vector<settings*> w_tabs;
    std::vector<settings*> m_tabs;

    for (auto* w : widgets_)
    {
        auto* tab = w->get_settings(settings_);
        if (tab)
            dlg.attach(tab);

        w_tabs.push_back(tab);
    }

    for (auto* m : modules_)
    {
        auto* tab = m->get_settings(settings_);
        if (tab) 
            dlg.attach(tab);

        m_tabs.push_back(tab);
    }

    dlg.organize();
    dlg.show(name);
    dlg.exec();

    const bool apply = 
       (dlg.result() == QDialog::Accepted);

    for (std::size_t i=0; i<widgets_.size(); ++i)
    {
        auto* widget = widgets_[i];
        auto* tab = w_tabs[i];
        if (apply && tab)
            widget->apply_settings(tab, settings_);

        if (tab)
            widget->free_settings(tab);
    }

    for (std::size_t i=0; i<modules_.size(); ++i)
    {
        auto* module = modules_[i];
        auto* tab = m_tabs[i];
        if (apply && tab)
            module->apply_settings(tab, settings_);

        if (tab)
            module->free_settings(tab);
    }    
}

void mainwindow::prepare_file_menu()
{
    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        ui_.menuFile->addAction(ui_.actionMinimize);
        ui_.menuFile->addSeparator();
    }

    for (auto* m : modules_)
        if (m->add_actions(*ui_.menuFile))
            ui_.menuFile->addSeparator();

    ui_.menuFile->addSeparator();
    ui_.menuFile->addAction(ui_.actionExit);        
}

void mainwindow::prepare_main_tab()
{
    ui_.mainTab->setCurrentIndex(0);            
}

QString mainwindow::select_download_folder()
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
    dir = QDir(dir).absolutePath();
    dir = QDir::toNativeSeparators(dir);

    if (!recents_.contains(dir))
        recents_.push_back(dir);

    if (recents_.size() > 10)
        recents_.pop_front();

    return dir;
}

QString mainwindow::select_save_nzb_folder()
{
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setWindowTitle(tr("Select save NZB folder"));
    dlg.setDirectory(recent_save_nzb_path_);
    if (dlg.exec() == QDialog::Rejected)
        return QString();

    auto dir = dlg.selectedFiles().first();

    recent_save_nzb_path_ = dir;

    return QDir::toNativeSeparators(dir);
}

QString mainwindow::select_nzb_file()
{
    const auto& file = QFileDialog::getOpenFileName(this,
        tr("Select NZB file"), recent_load_nzb_path_, "*.nzb");
    if (file.isEmpty())
        return "";

    recent_load_nzb_path_ = QFileInfo(file).absolutePath();
    return QDir::toNativeSeparators(file);
}

QString mainwindow::select_nzb_save_file(const QString& filename)
{
    const auto& file = QFileDialog::getSaveFileName(this,
        tr("Save NZB file"), recent_save_nzb_path_ + "/" + filename, "*.nzb");
    if (file.isEmpty())
        return "";

    recent_save_nzb_path_ = QFileInfo(file).absolutePath();
    return file;
}

QStringList mainwindow::get_recent_paths() const 
{
    return recents_;
}

quint32 mainwindow::choose_account(const QString& description)
{
    if (app::g_accounts->num_accounts() == 0)
    {
        auto account = app::g_accounts->suggest();
        DlgAccount dlg(this, account);
        if (dlg.exec() == QDialog::Rejected)
            return 0;

        app::g_accounts->set(account);
        return account.id;
    }

    auto* main = app::g_accounts->get_main_account();
    if (main)
        return main->id;

    QStringList names;

    const auto num_acc = app::g_accounts->num_accounts();
    for (std::size_t i=0; i<num_acc; ++i)
    {
        const auto& acc = app::g_accounts->get(i);
        names << acc.name;
    }
    DlgChoose dlg(this, names, description);
    if (dlg.exec() == QDialog::Rejected)
        return 0;

    const auto& acc_name = dlg.account();

    int account_index;
    for (account_index=0; account_index<names.size(); ++account_index)
    {
        if (names[account_index] == acc_name)
            break;
    }
    const auto& acc = app::g_accounts->get(account_index);

    if (dlg.remember())
        app::g_accounts->set_main_account(acc.id);

    return acc.id;
}

void mainwindow::update(mainwidget* widget)
{
    const auto index = ui_.mainTab->indexOf(widget);
    if (index == -1)
        return;

    ui_.mainTab->setTabText(index, widget->windowTitle());
    ui_.mainTab->setTabIcon(index, widget->windowIcon());
}

void mainwindow::show(const QString& title)
{
    const auto it = std::find_if(std::begin(widgets_), std::end(widgets_),
        [&](const mainwidget* widget) {
            return widget->windowTitle() == title;
        });
    if (it == std::end(widgets_))
        return;

    show(std::distance(std::begin(widgets_), it));
}

void mainwindow::show(mainwidget* widget)
{
    auto it = std::find(std::begin(widgets_), std::end(widgets_), widget);
    if (it == std::end(widgets_))
        return;

    show(std::distance(std::begin(widgets_), it));
}

void mainwindow::show(std::size_t index)
{
    auto* widget = widgets_[index];
    auto* action = actions_[index];

    // if already show, then do nothing
    auto pos = ui_.mainTab->indexOf(widget);
    if (pos != -1)
        return;

    action->setChecked(true);

    const auto& icon = widget->windowIcon();
    const auto& text = widget->windowTitle();
    ui_.mainTab->insertTab(index, widget, icon, text);
    build_window_menu();

}

void mainwindow::hide(mainwidget* widget)
{
    auto it = std::find(std::begin(widgets_), std::end(widgets_), widget);

    Q_ASSERT(it != std::end(widgets_));

    auto index = std::distance(std::begin(widgets_), it);

    hide(index);

}

void mainwindow::hide(std::size_t index)
{
    auto* widget = widgets_[index];
    auto* action = actions_[index];

    // if already not shown then do nothing
    auto pos = ui_.mainTab->indexOf(widget);
    if (pos == -1)
        return;

    action->setChecked(false);

    ui_.mainTab->removeTab(pos);
 
    build_window_menu();
}

void mainwindow::focus(mainwidget* ui)
{
    const auto index = ui_.mainTab->indexOf(ui);
    if (index == -1)
        return;

    ui_.mainTab->setCurrentIndex(index);
}

void mainwindow::closeEvent(QCloseEvent* event)
{
    DEBUG("Begin shutdown...");

    DEBUG("Block signals");
    refresh_timer_.stop();
    refresh_timer_.blockSignals(true);

    ui_.mainTab->blockSignals(true);
    ui_.mainToolBar->blockSignals(true);
    ui_.menuBar->blockSignals(true);

    for (auto* w : widgets_)
    {
        DEBUG(str("Shutting down: _1", w->windowTitle()));
        w->shutdown();
        DEBUG("Complete!");
    }

    for (auto* m : modules_)
    {
        const auto& info = m->information();
        DEBUG(str("Shutting down module: _1", info.name));
        m->shutdown();
        DEBUG("Complete");
    }

    DEBUG("Begin engine shutdown");

    // try to perform orderly engine shutdown.
    DlgShutdown dlg(this);
    dlg.setText(tr("Shutting down engine"));
    dlg.show();    

    QEventLoop loop;
    QObject::connect(app::g_engine, SIGNAL(shutdownComplete()), &loop, SLOT(quit()));
    if (!app::g_engine->shutdown())
        loop.exec();

    DEBUG("Begin save state");

    if (!savestate(&dlg))
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

void mainwindow::dragEnterEvent(QDragEnterEvent* event)
{
    DEBUG("dragEnterEvent");

    event->acceptProposedAction();
}

void mainwindow::dropEvent(QDropEvent* event)
{
    DEBUG("dropEvent");

    const auto* mime = event->mimeData();
    if (!mime->hasUrls())
        return;

    const auto& urls = mime->urls();
    for (int i=0; i<urls.size(); ++i)
    {
        const auto& name = urls[i].toLocalFile();
        const auto& info = QFileInfo(name);
        if (!info.isFile() || !info.exists())
            continue;

        DEBUG(str("processing _1 file", name));
        for (auto* w : widgets_)
            w->drop_file(name);

        for (auto* m : modules_)
            m->drop_file(name);
    }
}

void mainwindow::build_window_menu()
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

bool mainwindow::savestate(DlgShutdown* dlg)
{
    try
    {
        dlg->setText(tr("Saving accounts"));

        if (!app::g_accounts->savestate(settings_))
            return false;

        dlg->setText(tr("Saving settings"));

        if (!app::g_engine->savestate(settings_))
            return false;

        settings_.set("window", "width", width());
        settings_.set("window", "height", height());
        settings_.set("window", "x", x());
        settings_.set("window", "y", y());
        settings_.set("window", "show_toolbar", ui_.mainToolBar->isVisible());
        settings_.set("window", "show_statusbar", ui_.statusBar->isVisible()); 
        settings_.set("paths", "recents", recents_);
        settings_.set("paths", "save_nzb", recent_save_nzb_path_);
        settings_.set("paths", "load_nzb", recent_load_nzb_path_);

        // save tab visibility values
        // but only for permanent widgets, i.e. those that have actions.
        for (std::size_t i=0; i<actions_.size(); ++i)
        {
            const auto& text = widgets_[i]->windowTitle();
            const auto show  = actions_[i]->isChecked();
            settings_.set("window_visible_tabs", text, show);
        }

        // save widget states
        for (auto* w : widgets_)
        {
            if (!w->savestate(settings_))
                return false;
        }

        // save module state
        for (auto* m : modules_)
        {
            if (!m->savestate(settings_))
                return false;
        }

        const auto file = app::homedir::file("settings.json");
        const auto err  = settings_.save(file);
        if (err != QFile::NoError)
        {
            ERROR(str("Failed to save settings file _1, _2", file, err));
            return false;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        QMessageBox msg;
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Critical);
        msg.setText(tr("Critical error while saving application state:\n\r %1").arg(e.what()));
        msg.exec();
    }
    return false;
}

void mainwindow::on_mainTab_currentChanged(int index)
{
    //DEBUG(str("Current tab _1", index));

    if (current_)
        current_->deactivate();

    ui_.mainToolBar->clear();
    ui_.menuEdit->clear();

    if (index != -1)
    {
        auto* widget = static_cast<mainwidget*>(ui_.mainTab->widget(index)); 

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

    build_window_menu();
}

void mainwindow::on_mainTab_tabCloseRequested(int tab)
{
    auto* widget = static_cast<mainwidget*>(ui_.mainTab->widget(tab));

    ui_.mainTab->removeTab(tab);

    // find in the array
    auto it = std::find(std::begin(widgets_), std::end(widgets_), widget);

    Q_ASSERT(it != std::end(widgets_));

    const auto info = widget->information();
    if (info.permanent)
    {
        const auto index = std::distance(std::begin(widgets_), it);
        Q_ASSERT(index < actions_.size());

        auto* action = actions_[index];
        action->setChecked(false);
    }
    else
    {
        widget->close_widget();
        widgets_.erase(it);
    }
    build_window_menu();
}

void mainwindow::on_actionWindowClose_triggered()
{
    const auto cur = ui_.mainTab->currentIndex();
    if (cur == -1)
        return;

    on_mainTab_tabCloseRequested(cur);
}

void mainwindow::on_actionWindowNext_triggered()
{
    // cycle to next tab in the maintab

    const auto cur = ui_.mainTab->currentIndex();
    if (cur == -1)
        return;

    const auto size = ui_.mainTab->count();
    const auto next = (cur + 1) % size;
    ui_.mainTab->setCurrentIndex(next);
}

void mainwindow::on_actionWindowPrev_triggered()
{
    // cycle to previous tab in the maintab

    const auto cur = ui_.mainTab->currentIndex();
    if (cur == -1)
        return;

    const auto size = ui_.mainTab->count();
    const auto prev = (cur == 0) ? size - 1 : cur - 1;
    ui_.mainTab->setCurrentIndex(prev);
}

void mainwindow::on_actionHelp_triggered()
{
    openhelp("index.html");
}

void mainwindow::on_actionMinimize_triggered()
{
    DEBUG("Minimize to tray!");

    QMainWindow::hide();

    ui_.actionRestore->setEnabled(true);
    ui_.actionMinimize->setEnabled(false);
}

void mainwindow::on_actionRestore_triggered()
{
    DEBUG("Restore from tray");

    QMainWindow::show();

    ui_.actionRestore->setEnabled(false);
    ui_.actionMinimize->setEnabled(true);
}

void mainwindow::on_actionContextHelp_triggered()
{
    const auto* widget = static_cast<mainwidget*>(ui_.mainTab->currentWidget());
    if (widget == nullptr)
        return;

    const auto& info = widget->information();

    openhelp(info.helpurl);
}

void mainwindow::on_actionExit_triggered()
{
    // calling close will generate QCloseEvent which
    // will invoke the normal shutdown sequence.
    QMainWindow::close();
}

void mainwindow::on_actionSettings_triggered()
{
    show_setting("");
}

void mainwindow::on_actionAbout_triggered()
{
    DlgAbout dlg(this);
    dlg.exec();
}

void mainwindow::actionWindowToggleView_triggered()
{
    // the signal comes from the action object in
    // the view menu. the index is the index to the widgets array

    const auto* action = static_cast<QAction*>(sender());
    const auto index   = action->property("index").toInt();
    const bool visible = action->isChecked();

    Q_ASSERT(index < widgets_.size());
    Q_ASSERT(index < actions_.size());

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

void mainwindow::actionWindowFocus_triggered()
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

void mainwindow::actionTray_activated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Context)
        return;

    QMenu menu;
    menu.addAction(ui_.actionRestore);
    menu.addSeparator();
    menu.addAction(ui_.actionExit);
    menu.exec(QCursor::pos());
}

void mainwindow::timerWelcome_timeout()
{
    DlgWelcome dlg(this);
    dlg.exec();

    const auto add_account = dlg.add_account();
    const auto show_help   = dlg.open_guide();

    if (show_help)
        openhelp("quick.html");

    for (auto* w : widgets_)
        w->first_launch(add_account);

    for (auto* m : modules_)
        m->first_launch();
}

void mainwindow::timerRefresh_timeout()
{
    //DEBUG("refresh view timer!");
    app::g_engine->refresh();

    for (auto* w : widgets_)
        w->refresh();

    const auto freespace        = app::g_engine->get_free_disk_space();
    const auto downloads        = app::g_engine->get_download_path();
    const auto netspeed         = app::g_engine->get_download_speed();
    const auto bytes_downloaded = app::g_engine->get_bytes_downloaded();
    const auto bytes_queued     = app::g_engine->get_bytes_queued();
    const auto bytes_written    = app::g_engine->get_bytes_written();

    ui_.lblDiskFree->setText(str("_1 _2", downloads, app::size{freespace}));
    ui_.lblNetIO->setText(str("_1 _2",  app::speed { netspeed }, app::size {bytes_downloaded}));     
    ui_.lblDiskIO->setText(str("_1", app::size { bytes_written }));
    ui_.lblQueue->setText(str("_1", app::size { bytes_queued }));

    ui_.netGraph->addSample(netspeed);
}

} // gui
