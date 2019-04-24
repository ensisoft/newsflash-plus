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
#include "app/smtpclient.h"
#include "app/utility.h"

namespace gui
{

MainWindow* g_win;

MainWindow::MainWindow(app::Settings& s) : QMainWindow(nullptr), mSettings(s)
{
    mUI.setupUi(this);

    // set network monitor colors
    TinyGraph::colors greenish = {};
    greenish.fill    = QColor(47, 117, 29, 150);
    greenish.grad1   = QColor(232, 232, 232);
    greenish.grad2   = QColor(200, 200, 200);
    greenish.outline = QColor(97, 212, 55);
    mUI.netGraph->setColors(greenish);

    // put the various little widgets in their correct places
    mUI.statusBar->insertPermanentWidget(0, mUI.frmPoweroff);
    mUI.statusBar->insertPermanentWidget(1, mUI.frmProgress);
    mUI.statusBar->insertPermanentWidget(2, mUI.frmDiskWrite);
    mUI.statusBar->insertPermanentWidget(3, mUI.frmGraph);
    mUI.statusBar->insertPermanentWidget(4, mUI.frmKbs);
    mUI.frmPoweroff->setVisible(false);
    mUI.mainToolBar->setVisible(true);
    mUI.statusBar->setVisible(true);
    mUI.actionViewToolbar->setChecked(true);
    mUI.actionViewStatusbar->setChecked(true);
    mUI.progressBar->setTextVisible(false);

    // set platform specific (idiomatic) key shortcuts here.
    mUI.actionOpen->setShortcut(QKeySequence::Open);
    mUI.actionExit->setShortcut(QKeySequence::Quit);
    mUI.actionWindowClose->setShortcut(QKeySequence::Close);
    mUI.actionWindowNext->setShortcut(QKeySequence::Forward);
    mUI.actionWindowPrev->setShortcut(QKeySequence::Back);
    mUI.actionFind->setShortcut(QKeySequence::Find);
    mUI.actionFindNext->setShortcut(QKeySequence::FindNext);
    mUI.actionFindPrev->setShortcut(QKeySequence::FindPrevious);

    QObject::connect(&mRefreshTimer, SIGNAL(timeout()),
        this, SLOT(timerRefresh_timeout()));
    mRefreshTimer.setInterval(500);
    mRefreshTimer.start();

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

void MainWindow::attachWidget(MainWidget* widget)
{
    Q_ASSERT(!widget->parent());

    const int index  = mWidgets.size();
    const auto& text = widget->windowTitle();
    const auto& icon = widget->windowIcon();
    const auto& info = widget->getInformation();

    // Each permanent widget gets a view action.
    // Permanent widgets are those that instead of being closed
    // are just hidden and toggled in the View menu.
    QAction* action = mUI.menuView->addAction(text);
    action->setCheckable(true);
    action->setChecked(true);
    action->setProperty("index", index);

    QObject::connect(action, SIGNAL(triggered()), this,
        SLOT(actionWindowToggleView_triggered()));
    QObject::connect(widget, SIGNAL(updateMenu(MainWidget*)),
        this, SLOT(updateMenu(MainWidget*)));
    QObject::connect(widget, SIGNAL(showSettings(const QString&)),
        this, SLOT(showSettings(const QString&)));

    mWidgets.push_back(widget);
    mActions.push_back(action);
    widget->setProperty("permanent", true);
}

void MainWindow::attachSessionWidget(MainWidget* widget)
{
    Q_ASSERT(!widget->parent());

    widget->setProperty("permanent", false);
    QObject::connect(widget, SIGNAL(updateMenu(MainWidget*)),
        this, SLOT(updateMenu(MainWidget*)));
    QObject::connect(widget, SIGNAL(showSettings(const QString&)),
        this, SLOT(showSettings(const QString&)));

    mWidgets.push_back(widget);

    const auto& text = widget->windowTitle();
    const auto& icon = widget->windowIcon();

    const auto count = mUI.mainTab->count();
    mUI.mainTab->addTab(widget, icon, text);
    mUI.mainTab->setCurrentIndex(count);
}

void MainWindow::attachModule(MainModule* module)
{
    mModules.push_back(module);
}

void MainWindow::detachAllWidgets()
{
    mUI.mainTab->clear();

    for (auto* w : mWidgets)
        w->setParent(nullptr);

    mWidgets.clear();
}

void MainWindow::closeWidget(MainWidget* widget)
{
    const auto index = mUI.mainTab->indexOf(widget);
    if (index == -1)
        return;
    on_mainTab_tabCloseRequested(index);
}

void MainWindow::loadState()
{
    const bool first_launch = !mSettings.contains("window", "width");

    if (first_launch)
    {
        // if the settings file doesn't exist then we assume that this is the
        // first launch of the application and just setup for the user welcome
        QTimer::singleShot(500, this, SLOT(timerWelcome_timeout()));
    }
    else
    {
        const auto MediaTypeVersion = mSettings.get("version", "mediatype").toInt();
        const auto FileTypeVersion  = mSettings.get("version", "filetype").toInt();
        const auto SettingsVersion  = mSettings.get("version", "settings").toInt();
        if (MediaTypeVersion != app::MediaTypeVersion ||
            FileTypeVersion  != app::FileTypeVersion  ||
            SettingsVersion  != app::Settings::Version)
        {
            QTimer::singleShot(500, this, SLOT(timerMigrate_timeout()));
        }
    }

    const auto width  = mSettings.get("window", "width", 0);
    const auto height = mSettings.get("window", "height", 0);
    if (width && height)
    {
        DEBUG("MainWindow dimensions %1 x %2", width, height);
        resize(width, height);
    }

    const auto x = mSettings.get("window", "x", 0);
    const auto y = mSettings.get("window", "y", 0);

    QDesktopWidget desktop;
    auto screen = desktop.availableGeometry(desktop.primaryScreen());
    if (x < screen.width() - 50 && y < screen.height() - 50)
    {
        DEBUG("MainWindow position %1, %2", x, y);
        move(x, y);
    }

    const auto show_statbar = mSettings.get("window", "show_statusbar", true);
    const auto show_toolbar = mSettings.get("window", "show_toolbar", true);
    mUI.mainToolBar->setVisible(show_toolbar);
    mUI.statusBar->setVisible(show_statbar);
    mUI.actionViewToolbar->setChecked(show_toolbar);
    mUI.actionViewStatusbar->setChecked(show_statbar);

    mRecents = mSettings.get("paths", "recents").toStringList();
    mRecentSaveNZBPath = mSettings.get("paths", "save_nzb").toString();
    mRecentLoadNZBPath = mSettings.get("paths", "load_nzb").toString();
    mKeyCode = mSettings.get("license", "keycode").toString();

    const auto success = keygen::verify_code(mKeyCode);

    for (std::size_t i=0; i<mWidgets.size(); ++i)
    {
        const auto text = mWidgets[i]->objectName();
        Q_ASSERT(!text.isEmpty());
        const auto icon = mWidgets[i]->windowIcon();
        const auto info = mWidgets[i]->getInformation();
        const auto show = mSettings.get("window_visible_tabs", text, info.initiallyVisible);
        if (show)
        {
            const auto title = mWidgets[i]->windowTitle();
            mUI.mainTab->insertTab(i, mWidgets[i], icon, title);
        }
        if (i < mActions.size())
            mActions[i]->setChecked(show);

        mWidgets[i]->loadState(mSettings);
        mWidgets[i]->updateRegistration(success);
    }

    for (auto* m : mModules)
    {
        m->loadState(mSettings);
        m->updateRegistration(success);

        if (!first_launch)
            continue;

        MainWidget* search = m->openSearch();
        if (search)
        {
            attachSessionWidget(search);
        }
        MainWidget* rss = m->openRSSFeed();
        if (rss)
        {
            attachSessionWidget(rss);
        }
    }

    // load tabs that need to be recovered from previous session
    QStringList session_widget_file_list;
    session_widget_file_list = mSettings.get("session", "temp_file_list").toStringList();
    for (const auto& temp : session_widget_file_list)
    {
        app::Settings settings;
        const auto file = app::homedir::file("temp/" + temp + ".json");
        const auto err = settings.load(file);
        if (err != QFile::NoError)
        {
            ERROR("Failed to load state from %1, %2", file, err);
            continue;
        }
        const QString& module = settings.get("widget", "module").toString();
        for (auto* m : mModules)
        {
            MainWidget* widget = nullptr;
            // todo: fix the module name checking properly.
            // should store the name of the module which created the widget
            // and then see which one in the list matches.
            if (module == "Search")
                widget = m->openSearch();
            else if (module == "RSS")
                widget = m->openRSSFeed();

            if (widget)
            {
                widget->loadState(settings);
                attachSessionWidget(widget);
                break;
            }
        }
        QFile::remove(file);
    }

    app::g_engine->loadSession();
}

void MainWindow::startup()
{
    for (auto* widget : mWidgets)
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

    for (auto* w : mWidgets)
    {
        auto* tab = w->getSettings();
        if (tab)
            dlg.attach(tab);

        w_tabs.push_back(tab);
    }

    for (auto* m : mModules)
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

    for (std::size_t i=0; i<mWidgets.size(); ++i)
    {
        auto* widget = mWidgets[i];
        auto* tab = w_tabs[i];
        if (apply && tab)
            widget->applySettings(tab);

        if (tab)
            widget->freeSettings(tab);
    }

    for (std::size_t i=0; i<mModules.size(); ++i)
    {
        auto* module = mModules[i];
        auto* tab = m_tabs[i];
        if (apply && tab)
            module->applySettings(tab);

        if (tab)
            module->freeSettings(tab);
    }
}

void MainWindow::prepareFileMenu()
{
    mUI.menuFile->addAction(mUI.actionOpen);
    mUI.menuFile->addSeparator();
    mUI.menuFile->addAction(mUI.actionSearch);
    mUI.menuFile->addAction(mUI.actionRSS);
    mUI.menuFile->addSeparator();

    for (auto* m : mModules)
        if (m->addActions(*mUI.menuFile))
            mUI.menuFile->addSeparator();

    mUI.menuFile->addSeparator();
    mUI.menuFile->addAction(mUI.actionPoweroff);
    mUI.menuFile->addSeparator();
    mUI.menuFile->addAction(mUI.actionExit);
}

void MainWindow::prepareMainTab()
{
    mUI.mainTab->setCurrentIndex(0);
}

QString MainWindow::selectDownloadFolder()
{
    QString latest;
    if (!mRecents.isEmpty())
        latest = mRecents.back();

    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setWindowTitle(tr("Select download folder"));
    dlg.setDirectory(latest);
    if (dlg.exec() == QDialog::Rejected)
        return {};

    auto dir = dlg.selectedFiles().first();
    dir = QDir(dir).absolutePath();
    dir = QDir::toNativeSeparators(dir);

    if (!mRecents.contains(dir))
        mRecents.push_back(dir);

    if (mRecents.size() > 10)
        mRecents.pop_front();

    return dir;
}

QString MainWindow::selectSaveNzbFolder()
{
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setWindowTitle(tr("Select save NZB folder"));
    dlg.setDirectory(mRecentSaveNZBPath);
    if (dlg.exec() == QDialog::Rejected)
        return QString();

    auto dir = dlg.selectedFiles().first();

    mRecentSaveNZBPath = dir;

    return QDir::toNativeSeparators(dir);
}

QString MainWindow::selectNzbOpenFile()
{
    const auto& file = QFileDialog::getOpenFileName(this,
        tr("Select Newzbin file"), mRecentLoadNZBPath, "Newzbin Files (*.nzb)");
    if (file.isEmpty())
        return "";

    mRecentLoadNZBPath = QFileInfo(file).absolutePath();
    return QDir::toNativeSeparators(file);
}

QString MainWindow::selectNzbSaveFile(const QString& filename)
{
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setWindowTitle(tr("Save Newzbin File As ..."));
    dlg.setDirectory(mRecentSaveNZBPath);
    dlg.selectFile(filename);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    if (dlg.exec() == QDialog::Rejected)
        return {};

    const auto& files = dlg.selectedFiles();
    const auto& file  = files.first();

    QFileInfo info(file);
    mRecentSaveNZBPath = info.absolutePath();
    return QDir::toNativeSeparators(file);
}

QStringList MainWindow::getRecentPaths() const
{
    return mRecents;
}

std::size_t MainWindow::numWidgets() const
{
    return mWidgets.size();
}

MainWidget* MainWindow::getWidget(std::size_t i)
{
    BOUNDSCHECK(mWidgets, i);
    return mWidgets[i];
}

void MainWindow::messageReceived(const QString& message)
{
    QFileInfo info(message);
    if (!info.isFile() || !info.exists())
        return;

    DEBUG("Message received as a file path %1", message);

    for (auto* m : mModules)
    {
        MainWidget* widget = m->dropFile(message);
        if (widget)
        {
            attachSessionWidget(widget);
            break;
        }
    }
}

void MainWindow::updateMenu(MainWidget* widget)
{
    if (widget != mCurrentWidget)
        return;

    mUI.mainToolBar->clear();
    mUI.menuTemp->clear();

    widget->addActions(*mUI.mainToolBar);
    widget->addActions(*mUI.menuTemp);

    mUI.mainToolBar->addSeparator();
    mUI.mainToolBar->addAction(mUI.actionContextHelp);
}

void MainWindow::showSettings(const QString& tabName)
{
    showSetting(tabName);
}

void MainWindow::show(const QString& title)
{
    const auto it = std::find_if(std::begin(mWidgets), std::end(mWidgets),
        [&](const MainWidget* widget) {
            return widget->windowTitle() == title;
        });
    if (it == std::end(mWidgets))
        return;

    show(std::distance(std::begin(mWidgets), it));
}

void MainWindow::show(MainWidget* widget)
{
    auto it = std::find(std::begin(mWidgets), std::end(mWidgets), widget);
    if (it == std::end(mWidgets))
        return;

    show(std::distance(std::begin(mWidgets), it));
}

void MainWindow::show(std::size_t index)
{
    BOUNDSCHECK(mWidgets, index);
    BOUNDSCHECK(mActions, index);
    auto* widget = mWidgets[index];
    auto* action = mActions[index];

    // if already show, then do nothing
    auto pos = mUI.mainTab->indexOf(widget);
    if (pos != -1)
        return;

    action->setChecked(true);

    const auto& icon = widget->windowIcon();
    const auto& text = widget->windowTitle();
    mUI.mainTab->insertTab(index, widget, icon, text);
    prepareWindowMenu();

}

void MainWindow::hide(MainWidget* widget)
{
    auto it = std::find(std::begin(mWidgets), std::end(mWidgets), widget);

    Q_ASSERT(it != std::end(mWidgets));

    auto index = std::distance(std::begin(mWidgets), it);

    hide(index);

}

void MainWindow::hide(std::size_t index)
{
    auto* widget = mWidgets[index];
    auto* action = mActions[index];

    // if already not shown then do nothing
    auto pos = mUI.mainTab->indexOf(widget);
    if (pos == -1)
        return;

    action->setChecked(false);

    mUI.mainTab->removeTab(pos);

    prepareWindowMenu();
}

void MainWindow::focus(MainWidget* ui)
{
    const auto index = mUI.mainTab->indexOf(ui);
    if (index == -1)
        return;

    mUI.mainTab->setCurrentIndex(index);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    DEBUG("Block signals");
    mRefreshTimer.stop();
    mRefreshTimer.blockSignals(true);

    // try to perform orderly engine shutdown.
    DlgExit dialog(this);
    dialog.setText(tr("Disconnecting..."));
    dialog.show();

    {
        // first shutdown the engine, and cease all event processing.
        // once that is done we can take a snapshot of the current engine state
        // and save it somewhere in order to continue from the same state on the
        // next application run. secondly we save all the other application data.
        DEBUG("Begin engine shutdown...");
        QEventLoop loop;
        QObject::connect(app::g_engine, SIGNAL(shutdownComplete()), &loop, SLOT(quit()));
        if (!app::g_engine->shutdown())
            loop.exec();
    }

    {
        DEBUG("Begin smtp client shutdown ...");
        dialog.setText(tr("Shutting down SMTP client"));
        QEventLoop loop;
        QObject::connect(app::g_smtp, SIGNAL(shutdownComplete()), &loop, SLOT(quit()));
        app::g_smtp->startShutdown();
        loop.exec();
        app::g_smtp->shutdown();
    }


    dialog.setText(tr("Saving application data..."));

    // now try to persist all the state. this operation is allowed to fail.
    // if it does fail, then ask the user if he wants to continue quitting
    if (!saveState(&dialog))
    {
        mRefreshTimer.start();
        mRefreshTimer.blockSignals(false);
        dialog.close();
        event->ignore();
        return;
    }

    // these operations are not allowed to fail.

    DEBUG("Begin shutdown modules...");

    for (auto* w : mWidgets)
    {
        DEBUG("Shutdown %1", w->windowTitle());
        w->shutdown();
    }

    for (auto* m : mModules)
    {
        DEBUG("Shutdown %1", m->getInformation().name);
        m->shutdown();
    }

    dialog.close();

    // important. block signals, we're shutting down and don't want to
    // run the normal widget activation/deactivation code anymore.
    mUI.mainTab->blockSignals(true);
    mUI.mainToolBar->blockSignals(true);
    mUI.menuBar->blockSignals(true);
    mUI.mainTab->clear();

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

        for (auto* m : mModules)
        {
            MainWidget* widget = m->dropFile(name);
            if (widget)
            {
                attachSessionWidget(widget);
            }
        }

        for (size_t i=0; i<mWidgets.size(); ++i)
        {
            auto* widget = mWidgets[i];
            if (widget->dropFile(name))
            {
                const bool permanent = widget->property("permanent").toBool();
                if (permanent)
                {
                    show(i);
                    focusWidget(widget);
                }
            }
        }
    }
}

FindWidget* MainWindow::getFindWidget()
{
    auto* main = static_cast<MainWidget*>(mUI.mainTab->currentWidget());
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
    mUI.menuWindow->clear();

    const auto count = mUI.mainTab->count();
    const auto curr  = mUI.mainTab->currentIndex();
    for (int i=0; i<count; ++i)
    {
        const auto& text = mUI.mainTab->tabText(i);
        const auto& icon = mUI.mainTab->tabIcon(i);
        QAction* action  = mUI.menuWindow->addAction(icon, text);
        action->setCheckable(true);
        action->setChecked(i == curr);
        action->setProperty("tab-index", i);
        if (i < 9)
            action->setShortcut(QKeySequence(Qt::ALT | (Qt::Key_1 + i)));

        QObject::connect(action, SIGNAL(triggered()),
            this, SLOT(actionWindowFocus_triggered()));
    }
    // and this is in the window menu
    mUI.menuWindow->addSeparator();
    mUI.menuWindow->addAction(mUI.actionWindowClose);
    mUI.menuWindow->addAction(mUI.actionWindowNext);
    mUI.menuWindow->addAction(mUI.actionWindowPrev);
}

bool MainWindow::saveState(DlgExit* dlg)
{
    try
    {
        DEBUG("Saving application state...");
        // we clear the settings here completely. this makes it easy to purge all stale
        // data from the settings. for example if tools are modified and removed the state
        // is kept in memory and then persisted cleanly into the settings object on save.
        mSettings.clear();

        // todo: refactor this
        app::g_accounts->saveState(mSettings);
        app::g_engine->saveState(mSettings);
        app::g_tools->saveState(mSettings);
        app::g_history->saveState(mSettings);


        mSettings.set("window", "width", width());
        mSettings.set("window", "height", height());
        mSettings.set("window", "x", x());
        mSettings.set("window", "y", y());
        mSettings.set("window", "show_toolbar", mUI.mainToolBar->isVisible());
        mSettings.set("window", "show_statusbar", mUI.statusBar->isVisible());
        mSettings.set("paths", "recents", mRecents);
        mSettings.set("paths", "save_nzb", mRecentSaveNZBPath);
        mSettings.set("paths", "load_nzb", mRecentLoadNZBPath);
        mSettings.set("license", "keycode", mKeyCode);

        mSettings.set("version", "mediatype", app::MediaTypeVersion);
        mSettings.set("version", "filetype", app::FileTypeVersion);
        mSettings.set("version", "settings", app::Settings::Version);

        // save tab visibility values
        // but only for permanent widgets, i.e. those that have actions.
        for (std::size_t i=0; i<mActions.size(); ++i)
        {
            const auto& text = mWidgets[i]->objectName();
            Q_ASSERT(!text.isEmpty());
            const auto show  = mActions[i]->isChecked();
            mSettings.set("window_visible_tabs", text, show);
        }

        QStringList session_widget_state_files;

        // save widget states
        for (auto* widget : mWidgets)
        {
            DEBUG("Saving widget %1", widget->windowTitle());
            const bool bIsPermanent = widget->property("permanent").toBool();
            if (bIsPermanent)
            {
                widget->saveState(mSettings);
                continue;
            }
            const auto temp = app::generateRandomString();
            const auto path = app::homedir::file("/temp/");
            const auto file = app::homedir::file("/temp/" + temp + ".json");
            QDir dir;
            if (!dir.mkpath(path))
            {
                ERROR("Failed to create %1 folder.", path);
                throw std::runtime_error("failed to create temp folder");
            }

            app::Settings widget_settings;
            // todo: we should save the module name
            widget_settings.set("widget", "module", widget->windowTitle());
            widget->saveState(widget_settings);
            session_widget_state_files << temp;
            widget_settings.save(file);

        }
        mSettings.set("session", "temp_file_list", session_widget_state_files);

        // save module state
        for (auto* m : mModules)
        {
            DEBUG("Saving module %1", m->getInformation().name);
            m->saveState(mSettings);
        }

        const auto file = app::homedir::file("settings.json");
        const auto err  = mSettings.save(file);
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
    if (mCurrentWidget)
        mCurrentWidget->deactivate();

    mUI.mainToolBar->clear();
    mUI.menuTemp->clear();

    if (index != -1)
    {
        auto* widget = static_cast<MainWidget*>(mUI.mainTab->widget(index));

        widget->activate(mUI.mainTab);
        widget->addActions(*mUI.mainToolBar);
        widget->addActions(*mUI.menuTemp);

        auto* finder = widget->getFinder();
        mUI.menuEdit->setEnabled(finder != nullptr);

        auto title = widget->windowTitle();
        auto space = title.indexOf(" ");
        if (space != -1)
            title.resize(space);

        mUI.menuTemp->setTitle(title);
        mCurrentWidget = widget;
    }

    // add the stuff that is always in the edit menu
    mUI.mainToolBar->addSeparator();
    mUI.mainToolBar->addAction(mUI.actionContextHelp);

    prepareWindowMenu();
}

void MainWindow::on_mainTab_tabCloseRequested(int tab)
{
    const auto* widget   = static_cast<MainWidget*>(mUI.mainTab->widget(tab));
    const auto parent    = widget->property("parent-object");
    const auto permanent = widget->property("permanent").toBool();

    if (widget == mCurrentWidget)
        mCurrentWidget = nullptr;

    mUI.mainTab->removeTab(tab);

    // find in the array
    const auto it = std::find(std::begin(mWidgets), std::end(mWidgets), widget);

    ASSERT(it != std::end(mWidgets));
    ASSERT(*it == widget);

    if (permanent)
    {
        const auto index = std::distance(std::begin(mWidgets), it);
        BOUNDSCHECK(mActions, index);

        auto* action = mActions[index];
        action->setChecked(false);
    }
    else
    {
        delete widget;
        mWidgets.erase(it);
    }
    prepareWindowMenu();

    // see if we have a parent
    if (parent.isValid())
    {
        auto* p = parent.value<QObject*>();

        auto it = std::find_if(std::begin(mWidgets), std::end(mWidgets),
            [&](MainWidget* w) {
                return static_cast<QObject*>(w) == p;
            });
        if (it != std::end(mWidgets))
            focusWidget(*it);
    }
}

void MainWindow::on_actionWindowClose_triggered()
{
    const auto cur = mUI.mainTab->currentIndex();
    if (cur == -1)
        return;

    on_mainTab_tabCloseRequested(cur);
}

void MainWindow::on_actionWindowNext_triggered()
{
    // cycle to next tab in the maintab

    const auto cur = mUI.mainTab->currentIndex();
    if (cur == -1)
        return;

    const auto size = mUI.mainTab->count();
    const auto next = (cur + 1) % size;
    mUI.mainTab->setCurrentIndex(next);
}

void MainWindow::on_actionWindowPrev_triggered()
{
    // cycle to previous tab in the maintab

    const auto cur = mUI.mainTab->currentIndex();
    if (cur == -1)
        return;

    const auto size = mUI.mainTab->count();
    const auto prev = (cur == 0) ? size - 1 : cur - 1;
    mUI.mainTab->setCurrentIndex(prev);
}

void MainWindow::on_actionOpen_triggered()
{
    const auto& file = QFileDialog::getOpenFileName(this,
        tr("Select Newzbin file"), mRecentLoadNZBPath, "Newzbin Files (*.nzb)");
    if (file.isEmpty())
        return;

    mRecentLoadNZBPath = QFileInfo(file).absolutePath();

    for (auto* m : mModules)
    {
        MainWidget* widget = m->openFile(file);
        if (widget)
        {
            attachSessionWidget(widget);
            break;
        }
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

    for (auto& m : mModules)
        m->updateRegistration(success);

    for (auto& w : mWidgets)
        w->updateRegistration(success);

    mKeyCode = keycode;
}

void MainWindow::on_actionContextHelp_triggered()
{
    const auto* widget = static_cast<MainWidget*>(mUI.mainTab->currentWidget());
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
    // open first available search widget provided by some module
    for (auto* m : mModules)
    {
        MainWidget* widget = m->openSearch();
        if (widget)
        {
            attachSessionWidget(widget);
            break;
        }
    }
}

void MainWindow::on_actionRSS_triggered()
{
    // open first available RSS widget provided by some module
    for (auto* m : mModules)
    {
        MainWidget* widget = m->openRSSFeed();
        if (widget)
        {
            attachSessionWidget(widget);
            break;
        }
    }
}


void MainWindow::on_actionSettings_triggered()
{
    showSetting("");
}

void MainWindow::showMessage(const QString& message)
{
    mUI.statusBar->showMessage(message, 5000);
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
    bool shouldPoweroff = false;
    bool sendReport = false;

    for (;;)
    {
        DlgPoweroff dlg(this, sendReport);
        dlg.exec();
        sendReport = dlg.sendReport();
        if (dlg.configureSmtp())
        {
            showSetting("Email");
        }
        else
        {
            shouldPoweroff = dlg.isPoweroffEnabled();
            break;
        }
    }

    mUI.frmPoweroff->setVisible(shouldPoweroff);
}

void MainWindow::on_actionDonate_triggered()
{
    app::openWeb(NEWSFLASH_DONATE_URL);
}


void MainWindow::actionWindowToggleView_triggered()
{
    // the signal comes from the action object in
    // the view menu. the index is the index to the widgets array

    const auto* action = static_cast<QAction*>(sender());
    const auto index   = action->property("index").toInt();
    const bool visible = action->isChecked();

    BOUNDSCHECK(mWidgets, index);
    BOUNDSCHECK(mActions, index);

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

    mUI.mainTab->setCurrentIndex(tab_index);
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

    for (auto* w : mWidgets)
        w->firstLaunch();

    for (auto* m : mModules)
        m->firstLaunch();
}

void MainWindow::timerRefresh_timeout()
{
    //DEBUG("refresh view timer!");
    app::g_engine->refresh();

    for (auto* w : mWidgets)
    {
        const auto isActive = (w == mCurrentWidget);
        w->refresh(isActive);
    }

    // this is the easiest way to allow tabs to indicate new status.
    // by letting them change their window title/icon and then we periodidcally
    // update the main tab window.
    const auto numVisibleTabs = mUI.mainTab->count();
    for (int i=0; i<numVisibleTabs; ++i)
    {
        const auto* widget = static_cast<MainWidget*>(mUI.mainTab->widget(i));
        const auto& icon = widget->windowIcon();
        const auto& text = widget->windowTitle();
        mUI.mainTab->setTabText(i, text);
        mUI.mainTab->setTabIcon(i, icon);
    }

    const auto netspeed         = app::g_engine->getDownloadSpeed();
    const auto bytes_downloaded = app::g_engine->getBytesDownloaded();
    const auto bytes_queued     = app::g_engine->getBytesQueued();
    const auto bytes_ready      = app::g_engine->getBytesReady();
    const auto bytes_written    = app::g_engine->getBytesWritten();
    const auto bytes_remaining  = bytes_queued - bytes_ready;

    mUI.progressBar->setMinimum(0);
    mUI.progressBar->setMaximum(100);
    mUI.progressBar->setValue(0);
    if (bytes_remaining)
    {
        const double done = ((double)bytes_ready / (double)bytes_queued) * 100.0;
        mUI.progressBar->setValue((int)done);
    }
    mUI.progressBar->setTextVisible(bytes_remaining != 0);
    mUI.lblNetIO->setText(app::toString("%1 %2",  app::speed { netspeed }, app::size {bytes_downloaded}));
    mUI.lblDiskIO->setText(app::toString("%1", app::size { bytes_written }));
    mUI.lblQueue->setText(app::toString("%1", app::size { bytes_remaining }));

    mUI.netGraph->addSample(netspeed);
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
