// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define LOGTAG "mainwindow"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
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
#include "newsflash/warnpop.h"

#include "tools/keygen/keygen.h"

#include "mainwindow.h"
#include "mainwidget.h"
#include "mainmodule.h"
#include "settings.h"
#include "dlgwelcome.h"
#include "dlgsettings.h"
#include "dlgaccount.h"
#include "dlgexit.h"
#include "dlgabout.h"
#include "dlgfeedback.h"
#include "dlgpoweroff.h"
#include "dlgregister.h"
#include "findwidget.h"
#include "app/eventlog.h"
#include "app/debug.h"
#include "app/format.h"
#include "app/homedir.h"
#include "app/settings.h"
#include "app/accounts.h"
#include "app/engine.h"
#include "app/platform.h"
#include "app/tools.h"
#include "app/distdir.h"
#include "app/types.h"
#include "app/media.h"
#include "app/filetype.h"
#include "app/historydb.h"

namespace gui
{

MainWindow* g_win;

MainWindow::MainWindow(app::Settings& s) : QMainWindow(nullptr), current_(nullptr), settings_(s)
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
    //ui_.statusBar->insertPermanentWidget(1, ui_.frmFreeSpace);
    ui_.frmFreeSpace->setVisible(false);
    ui_.statusBar->insertPermanentWidget(1, ui_.frmDiskWrite);
    ui_.statusBar->insertPermanentWidget(2, ui_.frmGraph);
    ui_.statusBar->insertPermanentWidget(3, ui_.frmKbs);
    ui_.mainToolBar->setVisible(true);
    ui_.statusBar->setVisible(true);
    ui_.actionViewToolbar->setChecked(true);
    ui_.actionViewStatusbar->setChecked(true);
    ui_.progressBar->setTextVisible(false);

    ui_.actionOpen->setShortcut(QKeySequence::Open);
    ui_.actionExit->setShortcut(QKeySequence::Quit);
    ui_.actionSearch->setShortcut(QKeySequence::New);
    ui_.actionWindowClose->setShortcut(QKeySequence::Close);
    ui_.actionWindowNext->setShortcut(QKeySequence::Forward);
    ui_.actionWindowPrev->setShortcut(QKeySequence::Back);

    ui_.actionFind->setShortcut(QKeySequence::Find);
    ui_.actionFindNext->setShortcut(QKeySequence::FindNext);
    ui_.actionFindPrev->setShortcut(QKeySequence::FindPrevious);

    QObject::connect(&refresh_timer_, SIGNAL(timeout()),
        this, SLOT(timerRefresh_timeout()));
    refresh_timer_.setInterval(500);
    refresh_timer_.start();

    auto& eventLog = app::EventLog::get();
    QObject::connect(&eventLog, SIGNAL(newEvent(const app::Event&)),
        this, SLOT(displayNote(const app::Event&)));

    setWindowTitle(NEWSFLASH_TITLE);
    setAcceptDrops(true);

    DEBUG("MainWindow created");

}

MainWindow::~MainWindow()
{
    DEBUG("MainWindow deleted");
}

void MainWindow::showWindow()
{
    show();

    emit shown();
}

void MainWindow::attach(MainWidget* widget, bool permanent, bool loadstate)
{
    Q_ASSERT(!widget->parent());

    int index = widgets_.size();

    widgets_.push_back(widget);

    const auto& text = widget->windowTitle();
    const auto& icon = widget->windowIcon();
    const auto& info = widget->getInformation();

    // only permanent widgets get a view action.
    // permant widgets are those that instead of being closed
    // are just hidden and toggled in the View menu.
    if (permanent)
    {
        QAction* action = ui_.menuView->addAction(text);
        action->setCheckable(true);
        action->setChecked(true);
        action->setProperty("index", index);
        QObject::connect(action, SIGNAL(triggered()), this,
            SLOT(actionWindowToggleView_triggered()));

        actions_.push_back(action);
        widget->setProperty("permanent", true);
        if (loadstate)
            widget->loadState(settings_);
    }
    else
    {
        const auto count = ui_.mainTab->count();
        ui_.mainTab->addTab(widget, icon, text);
        ui_.mainTab->setCurrentIndex(count);
        widget->setProperty("permanent", false);
        if (loadstate)
        {
            auto it = std::find_if(std::begin(transient_), std::end(transient_),
                [=](const app::Settings& s) {
                    return s.name() == widget->objectName();
                });
            if (it == std::end(transient_))
                widget->loadState(settings_);
            else {
                auto& settings = *it;
                widget->loadState(settings);
            }
        }
    }

    QObject::connect(widget, SIGNAL(updateMenu(MainWidget*)),
        this, SLOT(updateMenu(MainWidget*)));
    QObject::connect(widget, SIGNAL(showSettings(MainWidget*)),
        this, SLOT(showSettings(MainWidget*)));
}

void MainWindow::attach(MainModule* module, bool loadstate)
{
    modules_.push_back(module);
}

void MainWindow::detachAllWidgets()
{
    ui_.mainTab->clear();

    for (auto* w : widgets_)
        w->setParent(nullptr);

    widgets_.clear();
}

void MainWindow::closeWidget(MainWidget* widget)
{
    const auto index = ui_.mainTab->indexOf(widget);
    if (index == -1)
        return;
    on_mainTab_tabCloseRequested(index);
}

void MainWindow::loadState()
{
    if (!settings_.contains("window", "width"))
    {
        // if the settings file doesn't exist then we assume that this is the
        // first launch of the application and just setup for the user welcome
        QTimer::singleShot(500, this, SLOT(timerWelcome_timeout()));
    }
    else
    {
        const auto MediaTypeVersion = settings_.get("version", "mediatype").toInt();
        const auto FileTypeVersion  = settings_.get("version", "filetype").toInt();
        if (MediaTypeVersion != app::MediaTypeVersion ||
            FileTypeVersion  != app::FileTypeVersion)
        {
            QTimer::singleShot(500, this, SLOT(timerMigrate_timeout()));
        }
    }

    const auto width  = settings_.get("window", "width", 0);
    const auto height = settings_.get("window", "height", 0);
    if (width && height)
    {
        DEBUG("MainWindow dimensions %1 x %2", width, height);
        resize(width, height);
    }

    const auto x = settings_.get("window", "x", 0);
    const auto y = settings_.get("window", "y", 0);

    QDesktopWidget desktop;
    auto screen = desktop.availableGeometry(desktop.primaryScreen());
    if (x < screen.width() - 50 && y < screen.height() - 50)
    {
        DEBUG("MainWindow position %1, %2", x, y);
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
    keyCode_ = settings_.get("license", "keycode").toString();

    const auto success = keygen::verify_code(keyCode_);

    for (std::size_t i=0; i<widgets_.size(); ++i)
    {
        const auto text = widgets_[i]->objectName();
        Q_ASSERT(!text.isEmpty());
        const auto icon = widgets_[i]->windowIcon();
        const auto info = widgets_[i]->getInformation();
        const auto show = settings_.get("window_visible_tabs", text, info.initiallyVisible);
        if (show)
        {
            const auto title = widgets_[i]->windowTitle();
            ui_.mainTab->insertTab(i, widgets_[i], icon, title);
        }
        if (i < actions_.size())
            actions_[i]->setChecked(show);

        widgets_[i]->loadState(settings_);
        widgets_[i]->updateRegistration(success);
    }

    for (auto* m : modules_)
    {
        m->loadState(settings_);
        m->updateRegistration(success);
    }

    app::g_engine->loadSession();
}

void MainWindow::startup()
{
    for (auto* widget : widgets_)
    {
        widget->startup();
    }
}

void MainWindow::focusWidget(const MainWidget* widget)
{
    focus(const_cast<MainWidget*>(widget));
}

void MainWindow::showSetting(const QString& name)
{
    DlgSettings dlg(this);

    std::vector<SettingsWidget*> w_tabs;
    std::vector<SettingsWidget*> m_tabs;

    for (auto* w : widgets_)
    {
        auto* tab = w->getSettings();
        if (tab)
            dlg.attach(tab);

        w_tabs.push_back(tab);
    }

    for (auto* m : modules_)
    {
        auto* tab = m->getSettings();
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
            widget->applySettings(tab);

        if (tab)
            widget->freeSettings(tab);
    }

    for (std::size_t i=0; i<modules_.size(); ++i)
    {
        auto* module = modules_[i];
        auto* tab = m_tabs[i];
        if (apply && tab)
            module->applySettings(tab);

        if (tab)
            module->freeSettings(tab);
    }
}

void MainWindow::prepareFileMenu()
{
    ui_.menuFile->addAction(ui_.actionOpen);
    ui_.menuFile->addSeparator();
    ui_.menuFile->addAction(ui_.actionSearch);
    ui_.menuFile->addSeparator();

    for (auto* m : modules_)
        if (m->addActions(*ui_.menuFile))
            ui_.menuFile->addSeparator();

    ui_.menuFile->addSeparator();
    ui_.menuFile->addAction(ui_.actionPoweroff);
    ui_.menuFile->addSeparator();
    ui_.menuFile->addAction(ui_.actionExit);
}

void MainWindow::prepareMainTab()
{
    ui_.mainTab->setCurrentIndex(0);
}

QString MainWindow::selectDownloadFolder()
{
    QString latest;
    if (!recents_.isEmpty())
        latest = recents_.back();

    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setWindowTitle(tr("Select download folder"));
    dlg.setDirectory(latest);
    if (dlg.exec() == QDialog::Rejected)
        return {};

    auto dir = dlg.selectedFiles().first();
    dir = QDir(dir).absolutePath();
    dir = QDir::toNativeSeparators(dir);

    if (!recents_.contains(dir))
        recents_.push_back(dir);

    if (recents_.size() > 10)
        recents_.pop_front();

    return dir;
}

QString MainWindow::selectSaveNzbFolder()
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

QString MainWindow::selectNzbOpenFile()
{
    const auto& file = QFileDialog::getOpenFileName(this,
        tr("Select Newzbin file"), recent_load_nzb_path_, "Newzbin Files (*.nzb)");
    if (file.isEmpty())
        return "";

    recent_load_nzb_path_ = QFileInfo(file).absolutePath();
    return QDir::toNativeSeparators(file);
}

QString MainWindow::selectNzbSaveFile(const QString& filename)
{
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setWindowTitle(tr("Save Newzbin File As ..."));
    dlg.setDirectory(recent_save_nzb_path_);
    dlg.selectFile(filename);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    if (dlg.exec() == QDialog::Rejected)
        return {};

    const auto& files = dlg.selectedFiles();
    const auto& file  = files.first();

    QFileInfo info(file);
    recent_save_nzb_path_ = info.absolutePath();
    return QDir::toNativeSeparators(file);
}

QStringList MainWindow::getRecentPaths() const
{
    return recents_;
}

std::size_t MainWindow::numWidgets() const
{
    return widgets_.size();
}

MainWidget* MainWindow::getWidget(std::size_t i)
{
    BOUNDSCHECK(widgets_, i);
    return widgets_[i];
}

void MainWindow::messageReceived(const QString& message)
{
    QFileInfo info(message);
    if (!info.isFile() || !info.exists())
        return;

    DEBUG("Message received as a file path %1", message);

    for (auto* m : modules_)
    {
        MainWidget* widget = m->dropFile(message);
        if (widget)
            attach(widget, false, true);
    }
}

void MainWindow::updateMenu(MainWidget* widget)
{
    if (widget != current_)
        return;

    ui_.mainToolBar->clear();
    ui_.menuTemp->clear();

    widget->addActions(*ui_.mainToolBar);
    widget->addActions(*ui_.menuTemp);

    ui_.mainToolBar->addSeparator();
    ui_.mainToolBar->addAction(ui_.actionContextHelp);
}

void MainWindow::showSettings(MainWidget* widget)
{
    showSetting(widget->windowTitle());
}

void MainWindow::show(const QString& title)
{
    const auto it = std::find_if(std::begin(widgets_), std::end(widgets_),
        [&](const MainWidget* widget) {
            return widget->windowTitle() == title;
        });
    if (it == std::end(widgets_))
        return;

    show(std::distance(std::begin(widgets_), it));
}

void MainWindow::show(MainWidget* widget)
{
    auto it = std::find(std::begin(widgets_), std::end(widgets_), widget);
    if (it == std::end(widgets_))
        return;

    show(std::distance(std::begin(widgets_), it));
}

void MainWindow::show(std::size_t index)
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
    prepareWindowMenu();

}

void MainWindow::hide(MainWidget* widget)
{
    auto it = std::find(std::begin(widgets_), std::end(widgets_), widget);

    Q_ASSERT(it != std::end(widgets_));

    auto index = std::distance(std::begin(widgets_), it);

    hide(index);

}

void MainWindow::hide(std::size_t index)
{
    auto* widget = widgets_[index];
    auto* action = actions_[index];

    // if already not shown then do nothing
    auto pos = ui_.mainTab->indexOf(widget);
    if (pos == -1)
        return;

    action->setChecked(false);

    ui_.mainTab->removeTab(pos);

    prepareWindowMenu();
}

void MainWindow::focus(MainWidget* ui)
{
    const auto index = ui_.mainTab->indexOf(ui);
    if (index == -1)
        return;

    ui_.mainTab->setCurrentIndex(index);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    DEBUG("Block signals");
    refresh_timer_.stop();
    refresh_timer_.blockSignals(true);

    // try to perform orderly engine shutdown.
    DlgExit dialog(this);
    dialog.setText(tr("Disconnecting..."));
    dialog.show();

    // first shutdown the engine, and cease all event processing.
    // once that is done we can take a snapshot of the current engine state
    // and save it somewhere in order to continue from the same state on the
    // next application run. secondly we save all the other application data.
    DEBUG("Begin engine shutdown...");

    QEventLoop loop;
    QObject::connect(app::g_engine, SIGNAL(shutdownComplete()), &loop, SLOT(quit()));
    if (!app::g_engine->shutdown())
       loop.exec();

    dialog.setText(tr("Saving application data..."));

    // now try to persist all the state. this operation is allowed to fail.
    // if it does fail, then ask the user if he wants to continue quitting
    if (!saveState(&dialog))
    {
        refresh_timer_.start();
        refresh_timer_.blockSignals(false);
        dialog.close();
        event->ignore();
        return;
    }

    // these operations are not allowed to fail.

    DEBUG("Begin shutdown modules...");

    for (auto* w : widgets_)
    {
        DEBUG("Shutdown %1", w->windowTitle());
        w->shutdown();
    }

    for (auto* m : modules_)
    {
        DEBUG("Shutdown %1", m->getInformation().name);
        m->shutdown();
    }

    dialog.close();

    // important. block signals, we're shutting down and don't want to
    // run the normal widget activation/deactivation code anymore.
    ui_.mainTab->blockSignals(true);
    ui_.mainToolBar->blockSignals(true);
    ui_.menuBar->blockSignals(true);
    ui_.mainTab->clear();

    event->accept();
    DEBUG("Shutdown complete!");

    emit closed();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    DEBUG("dragEnterEvent");

    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
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

        DEBUG("processing %1 file", name);

        for (auto* m : modules_)
        {
            MainWidget* widget = m->dropFile(name);
            if (widget)
                attach(widget, false, true);
        }
    }
}

FindWidget* MainWindow::getFindWidget()
{
    auto* main = static_cast<MainWidget*>(ui_.mainTab->currentWidget());
    if (main == nullptr)
        return nullptr;

    Finder* finder = main->getFinder();
    if (finder == nullptr)
        return nullptr;

    FindWidget* findWidget = nullptr;

    QList<QWidget*> kids = main->findChildren<QWidget*>("findWidget");
    if (kids.isEmpty())
    {
        auto* layout = main->layout();
        Q_ASSERT(layout &&
            "The MainWidget doesn't have appropriate layout object.");
        findWidget = new FindWidget(main, *finder);
        findWidget->setObjectName("findWidget");
        layout->addWidget(findWidget);
    }
    else
    {
        findWidget = static_cast<FindWidget*>(kids[0]);
    }
    return findWidget;
}

void MainWindow::openHelp(const QString& page)
{
    const auto& help = app::distdir::path("help/" + page);
    QFileInfo info(help);
    if (info.exists())
        app::openFile(help);
    else app::openWeb("http://www.ensisoft.com/help/" + page);
}


void MainWindow::prepareWindowMenu()
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

bool MainWindow::saveState(DlgExit* dlg)
{
    try
    {
        DEBUG("Saving application state...");
        // we clear the settings here completely. this makes it easy to purge all stale
        // data from the settings. for example if tools are modified and removed the state
        // is kept in memory and then persisted cleanly into the settings object on save.
        settings_.clear();
        for (const auto& a : transient_)
            settings_.merge(a);

        // todo: refactor this
        app::g_accounts->saveState(settings_);
        app::g_engine->saveState(settings_);
        app::g_tools->saveState(settings_);
        app::g_history->saveState(settings_);


        settings_.set("window", "width", width());
        settings_.set("window", "height", height());
        settings_.set("window", "x", x());
        settings_.set("window", "y", y());
        settings_.set("window", "show_toolbar", ui_.mainToolBar->isVisible());
        settings_.set("window", "show_statusbar", ui_.statusBar->isVisible());
        settings_.set("paths", "recents", recents_);
        settings_.set("paths", "save_nzb", recent_save_nzb_path_);
        settings_.set("paths", "load_nzb", recent_load_nzb_path_);
        settings_.set("license", "keycode", keyCode_);

        settings_.set("version", "mediatype", app::MediaTypeVersion);
        settings_.set("version", "filetype", app::FileTypeVersion);

        // save tab visibility values
        // but only for permanent widgets, i.e. those that have actions.
        for (std::size_t i=0; i<actions_.size(); ++i)
        {
            const auto& text = widgets_[i]->objectName();
            Q_ASSERT(!text.isEmpty());
            const auto show  = actions_[i]->isChecked();
            settings_.set("window_visible_tabs", text, show);
        }

        // save widget states
        for (auto* w : widgets_)
        {
            DEBUG("Saving widget %1", w->windowTitle());
            w->saveState(settings_);
        }

        // save module state
        for (auto* m : modules_)
        {
            DEBUG("Saving module %1", m->getInformation().name);
            m->saveState(settings_);
        }

        const auto file = app::homedir::file("settings.json");
        const auto err  = settings_.save(file);
        if (err != QFile::NoError)
            throw std::runtime_error("failed to save settings in settings.json");

        app::g_engine->saveSession();
    }
    catch (const std::exception& e)
    {
        QMessageBox msg(dlg);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setIcon(QMessageBox::Critical);
        msg.setText(tr("Error while trying to save application state.\r\n%1\r\n"
                       "Do you want to continue (data might be lost)").arg(e.what()));
        if (msg.exec() == QMessageBox::No)
            return false;
    }
    return true;
}

void MainWindow::on_mainTab_currentChanged(int index)
{
    if (current_)
        current_->deactivate();

    ui_.mainToolBar->clear();
    ui_.menuTemp->clear();

    if (index != -1)
    {
        auto* widget = static_cast<MainWidget*>(ui_.mainTab->widget(index));

        widget->activate(ui_.mainTab);
        widget->addActions(*ui_.mainToolBar);
        widget->addActions(*ui_.menuTemp);

        auto* finder = widget->getFinder();
        ui_.menuEdit->setEnabled(finder != nullptr);

        auto title = widget->windowTitle();
        auto space = title.indexOf(" ");
        if (space != -1)
            title.resize(space);

        ui_.menuTemp->setTitle(title);
        current_ = widget;
    }

    // add the stuff that is always in the edit menu
    ui_.mainToolBar->addSeparator();
    ui_.mainToolBar->addAction(ui_.actionContextHelp);

    prepareWindowMenu();
}

void MainWindow::on_mainTab_tabCloseRequested(int tab)
{
    auto* widget = static_cast<MainWidget*>(ui_.mainTab->widget(tab));
    auto parent  = widget->property("parent-object");

    ui_.mainTab->removeTab(tab);

    // find in the array
    auto it = std::find(std::begin(widgets_), std::end(widgets_), widget);

    Q_ASSERT(it != std::end(widgets_));
    Q_ASSERT(*it == widget);

    auto permanent = widget->property("permanent").toBool();

    if (permanent)
    {
        const auto index = std::distance(std::begin(widgets_), it);
        BOUNDSCHECK(actions_, index);

        auto* action = actions_[index];
        action->setChecked(false);
    }
    else
    {
        // we have transient and permanent settings. the reason being
        // that we want to clear the settings saved on the disk from
        // stale data. I.e. if there's an object such as an account
        // that was deleted we want this data to be gone from the settings.json.
        // the easiest way to do this is just to clear the settings object
        // and then have all the modules persist their *current* state
        // into the settings and then write this out to the disk.
        // however this does not work for temporary widgets that are deleted
        // during program run because and which are not around anymore at the time
        // when the settings are saved.

        // so in order to maintain clean set of settings we have a separate
        // settings object for each type of transient widgets
        // which we then combine with the permanent settings to create the
        // final settings object saved to the file

        auto sit = std::find_if(std::begin(transient_), std::end(transient_),
            [=](const app::Settings& s) {
                return s.name() == widget->objectName();
            });
        if (sit == std::end(transient_))
        {
            app::Settings s;
            s.setName(widget->objectName());
            sit = transient_.insert(transient_.end(), s);
        }
        auto& settings = *sit;
        settings.clear();

        widget->saveState(settings);

        delete widget;
        widgets_.erase(it);
    }
    prepareWindowMenu();

    // see if we have a parent
    if (parent.isValid())
    {
        auto* p = parent.value<QObject*>();

        auto it = std::find_if(std::begin(widgets_), std::end(widgets_),
            [&](MainWidget* w) {
                return static_cast<QObject*>(w) == p;
            });
        if (it != std::end(widgets_))
            focusWidget(*it);
    }
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

void MainWindow::on_actionOpen_triggered()
{
    const auto& file = QFileDialog::getOpenFileName(this,
        tr("Select Newzbin file"), recent_load_nzb_path_, "Newzbin Files (*.nzb)");
    if (file.isEmpty())
        return;

    recent_load_nzb_path_ = QFileInfo(file).absolutePath();

    for (auto* m : modules_)
    {
        MainWidget* widget = m->openFile(file);
        if (widget)
            attach(widget, false, true);
    }
}

void MainWindow::on_actionHelp_triggered()
{
    openHelp("index.html");
}

void MainWindow::on_actionRegister_triggered()
{
    DlgRegister dlg(this);
    if (dlg.exec() == QDialog::Rejected)
        return;

    const auto& keycode = dlg.registrationCode();
    const auto success  = keygen::verify_code(keycode);
    if (!success)
    {
        QMessageBox::critical(this,
            tr("Registration failed"),
            tr("The registration was unsuccesful.\n"
               "Please make sure that you've input the key correctly."));
        return;
    }

    for (auto& m : modules_)
        m->updateRegistration(success);

    for (auto& w : widgets_)
        w->updateRegistration(success);

    keyCode_ = keycode;
}

void MainWindow::on_actionContextHelp_triggered()
{
    const auto* widget = static_cast<MainWidget*>(ui_.mainTab->currentWidget());
    if (widget == nullptr)
        return;

    const auto& info = widget->getInformation();
    const auto& help = info.helpurl;
    openHelp(help);
}

void MainWindow::on_actionExit_triggered()
{
    // calling close will generate QCloseEvent which
    // will invoke the normal shutdown sequence.
    QMainWindow::close();
}

void MainWindow::on_actionFind_triggered()
{
    auto* finder = getFindWidget();
    if (!finder) return;
    finder->show();
    finder->find();
}

void MainWindow::on_actionFindNext_triggered()
{
    auto* finder = getFindWidget();
    if (!finder) return;
    finder->findNext();
}

void MainWindow::on_actionFindPrev_triggered()
{
    auto* finder = getFindWidget();
    if (!finder) return;
    finder->findPrev();
}

void MainWindow::on_actionFindClose_triggered()
{
    auto* finder = getFindWidget();
    if (!finder) return;
    finder->close();
}

void MainWindow::on_actionSearch_triggered()
{
    for (auto* m : modules_)
    {
        MainWidget* widget = m->openSearch();
        if (widget)
            attach(widget, false, true);
    }
}


void MainWindow::on_actionSettings_triggered()
{
    showSetting("");
}

void MainWindow::showMessage(const QString& message)
{
    ui_.statusBar->showMessage(message, 5000);
}

void MainWindow::updateLicense()
{
    on_actionRegister_triggered();
}

void MainWindow::on_actionAbout_triggered()
{
    DlgAbout dlg(this);
    dlg.exec();
}

void MainWindow::on_actionViewForum_triggered()
{
    app::openWeb("http://www.ensisoft.com/forum");
}

void MainWindow::on_actionReportBug_triggered()
{
    DlgFeedback dlg(this, DlgFeedback::mode::BugReport);
    dlg.exec();
}

void MainWindow::on_actionSendFeedback_triggered()
{
    DlgFeedback dlg(this, DlgFeedback::mode::FeedBack);
    dlg.exec();
}

void MainWindow::on_actionRequestFeature_triggered()
{
    DlgFeedback dlg(this, DlgFeedback::mode::FeatureRequest);
    dlg.exec();
}

void MainWindow::on_actionRequestLicense_triggered()
{
    DlgFeedback dlg(this, DlgFeedback::mode::LicenseRequest);
    dlg.exec();
}
void MainWindow::on_actionPoweroff_triggered()
{
    DlgPoweroff dlg(this);
    dlg.exec();
}

void MainWindow::on_actionDonate_triggered()
{
    app::openWeb("http://www.ensisoft.com/donate");
}


void MainWindow::actionWindowToggleView_triggered()
{
    // the signal comes from the action object in
    // the view menu. the index is the index to the widgets array

    const auto* action = static_cast<QAction*>(sender());
    const auto index   = action->property("index").toInt();
    const bool visible = action->isChecked();

    BOUNDSCHECK(widgets_, index);
    BOUNDSCHECK(actions_, index);

    if (visible)
    {
        show(index);
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

void MainWindow::timerWelcome_timeout()
{
    DlgWelcome dlg(this);
    dlg.exec();

    const auto addAccount = dlg.add_account();
    const auto showHelp   = dlg.open_guide();

    if (showHelp)
    {
        openHelp("quick.html");
    }

    if (addAccount)
    {
        auto account = app::g_accounts->suggestAccount();
        DlgAccount dlg(this, account, true);
        if (dlg.exec() == QDialog::Accepted)
            app::g_accounts->setAccount(account);
    }

    for (auto* w : widgets_)
        w->firstLaunch();

    for (auto* m : modules_)
        m->firstLaunch();
}

void MainWindow::timerRefresh_timeout()
{
    //DEBUG("refresh view timer!");
    app::g_engine->refresh();

    for (auto* w : widgets_)
    {
        const auto isActive = (w == current_);
        w->refresh(isActive);
    }

    // this is the easiest way to allow tabs to indicate new status.
    // by letting them change their window title/icon and then we periodidcally
    // update the main tab window.
    const auto numVisibleTabs = ui_.mainTab->count();
    for (int i=0; i<numVisibleTabs; ++i)
    {
        const auto* widget = static_cast<MainWidget*>(ui_.mainTab->widget(i));
        const auto& icon = widget->windowIcon();
        const auto& text = widget->windowTitle();
        ui_.mainTab->setTabText(i, text);
        ui_.mainTab->setTabIcon(i, icon);
    }

    const auto netspeed         = app::g_engine->getDownloadSpeed();
    const auto bytes_downloaded = app::g_engine->getBytesDownloaded();
    const auto bytes_queued     = app::g_engine->getBytesQueued();
    const auto bytes_ready      = app::g_engine->getBytesReady();
    const auto bytes_written    = app::g_engine->getBytesWritten();
    const auto bytes_remaining  = bytes_queued - bytes_ready;

    ui_.progressBar->setMinimum(0);
    ui_.progressBar->setMaximum(100);
    ui_.progressBar->setValue(0);
    if (bytes_remaining)
    {
        const double done = ((double)bytes_ready / (double)bytes_queued) * 100.0;
        ui_.progressBar->setValue((int)done);
    }
    ui_.progressBar->setTextVisible(bytes_remaining != 0);
    ui_.lblNetIO->setText(app::toString("%1 %2",  app::speed { netspeed }, app::size {bytes_downloaded}));
    ui_.lblDiskIO->setText(app::toString("%1", app::size { bytes_written }));
    ui_.lblQueue->setText(app::toString("%1", app::size { bytes_remaining }));

    ui_.netGraph->addSample(netspeed);
}

void MainWindow::timerMigrate_timeout()
{
    QMessageBox::information(this,
        "Settings Update",
        "Your old settings have been migrated to current version.\n"
        "Some of the settings might require adjustment.");
}

void MainWindow::displayNote(const app::Event& event)
{
    if (event.type == app::Event::Type::Note)
        showMessage(event.message);
}

} // gui
