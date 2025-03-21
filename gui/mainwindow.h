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

#pragma once

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QMainWindow>
#  include <QTimer>
#  include <QList>
#  include "ui_mainwindow.h"
#include "newsflash/warnpop.h"

#include <vector>
#include <memory>

#include "app/settings.h"

class QIcon;
class QAction;
class QCloseEvent;
class QCoreApplication;

namespace app {
    class Settings;
    struct Event;
} // app

namespace gui
{
    class FindWidget;
    class MainWidget;
    class MainModule;
    class DlgShutdown;
    class DlgExit;

    // application's mainwindow. mainwindow manages the mainwidgets and
    // provides core GUI functionality that for example displaying common dialogs
    // for opening and saving application specific files (such as NZB).
    // it features a TabWidget that hosts between the mainwidgets.
    // Actions such as shutdown, drag&drop etc are handled initially
    // in the mainwindow and then propagated down to mainwidgets for handling.
    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        using QMainWindow::show;

        // ctor/dtor
        MainWindow(app::Settings& settings);
       ~MainWindow();

        void showWindow();

        // Attach a new permanent MainWidget to the window and display it.
        // The ownership of the widget remains with the caller.
        void attachWidget(MainWidget* widget);

        // attach a temporary session widget.
        void attachSessionWidget(MainWidget* widget);

        // attach a new module to the mainwindow.
        // ownership of the module remains with the caller.
        void attachModule(MainModule* module);

        // detach all widgets
        void detachAllWidgets();

        void closeWidget(MainWidget* widget);

        // load previously stored application/gui state.
        void loadState();

        void startup();

        void focusWidget(const MainWidget* widget);

        // show message in the window's message area
        void showMessage(const QString& message);

        void updateLicense();

        void prepareFileMenu();
        void prepareWindowMenu();
        void prepareMainTab();

        // perform a common action and select a download folder for some content.
        QString selectDownloadFolder();

        // select a folder for storing a nzb file into.
        QString selectSaveNzbFolder();

        // select a NZB file for opening.
        QString selectNzbOpenFile();

        // select NZB file for saving data
        QString selectNzbSaveFile(const QString& filename);

        QStringList getRecentPaths() const;

        std::size_t numWidgets() const;

        MainWidget* getWidget(std::size_t i);

        void showSetting(const QString& name);

    public slots:
        void messageReceived(const QString& msg);

    signals:
        void closed();
        void shown();

    private:
        void show(const QString& name);
        void show(MainWidget* widget);
        void show(std::size_t index);
        void hide(MainWidget* widget);
        void hide(std::size_t index);
        void focus(MainWidget* widget);
        bool saveState(DlgExit* dlg);
        void closeEvent(QCloseEvent* event);
        void dragEnterEvent(QDragEnterEvent* event);
        void dropEvent(QDropEvent* event);
        FindWidget* getFindWidget();
        void openHelp(const QString& page);

    private slots:
        void on_mainTab_currentChanged(int index);
        void on_mainTab_tabCloseRequested(int index);
        void on_actionContextHelp_triggered();
        void on_actionWindowClose_triggered();
        void on_actionWindowNext_triggered();
        void on_actionWindowPrev_triggered();
        void on_actionOpen_triggered();
        void on_actionHelp_triggered();
        void on_actionRegister_triggered();
        void on_actionExit_triggered();
        void on_actionFind_triggered();
        void on_actionFindNext_triggered();
        void on_actionFindPrev_triggered();
        void on_actionFindClose_triggered();
        void on_actionSearch_triggered();
        void on_actionRSS_triggered();
        void on_actionSettings_triggered();
        void on_actionAbout_triggered();
        void on_actionViewForum_triggered();
        void on_actionReportBug_triggered();
        void on_actionSendFeedback_triggered();
        void on_actionRequestFeature_triggered();
        void on_actionRequestLicense_triggered();
        void on_actionPoweroff_triggered();
        void on_actionDonate_triggered();
        void actionWindowToggleView_triggered();
        void actionWindowFocus_triggered();
        void timerWelcome_timeout();
        void timerRefresh_timeout();
        void timerMigrate_timeout();
        void displayNote(const app::Event& event);
        void updateMenu(MainWidget* widget);
        void showSettings(const QString& widget);

    private:
        // main UI object
        Ui::MainWindow mUI;

    private:
        app::Settings& mSettings;
        std::vector<MainModule*> mModules;
        std::vector<MainWidget*> mWidgets;
        std::vector<QAction*>    mActions;
        MainWidget* mCurrentWidget = nullptr;

    private:
        QStringList mRecents;
        QString mRecentSaveNZBPath;
        QString mRecentLoadNZBPath;
        QString mKeyCode;
    private:
        QTimer  mRefreshTimer;

    };

    extern MainWindow* g_win;

} // gui


