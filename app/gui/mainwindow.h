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

#pragma once

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QMainWindow>
#  include <QTimer>
#  include <QList>
#  include "ui_mainwindow.h"
#include <newsflash/warnpop.h>
#include <vector>
#include <memory>

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
    class MainWidget;
    class MainModule;
    class DlgShutdown;

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
       
        // attach a new widget to the mainwindow and display it.
        // ownership of the object remains with the caller.
        void attach(MainWidget* widget, bool loadstate = false);

        // attach a new module to the mainwindow.
        // ownership of the module remains with the caller.
        void attach(MainModule* module, bool loadstate = false);

        // update the widget's display state
        //void update(MainWidget* widget);
    

        // detach all widgets
        void detachAllWidgets();

        // load previously stored application/gui state.
        void loadState();

        // change application focus to the widget with the matching name      
        void showWidget(const QString& name);

        // open and show the application settings and select the
        // matching settings page.
        void showSetting(const QString& name);

        // show message in the window's message area
        void showMessage(const QString& message);

        void prepareFileMenu();
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

        quint32 chooseAccount(const QString& description);

   private:
        void show(const QString& name);
        void show(MainWidget* widget);
        void show(std::size_t index);
        void hide(MainWidget* widget);        
        void hide(std::size_t index);
        void focus(MainWidget* widget);    
        void buildWindowMenu();
        bool saveState(DlgShutdown* dlg);
        void closeEvent(QCloseEvent* event);        
        void dragEnterEvent(QDragEnterEvent* event);
        void dropEvent(QDropEvent* event);
        
    private slots:
        void on_mainTab_currentChanged(int index);
        void on_mainTab_tabCloseRequested(int index);
        void on_actionContextHelp_triggered();
        void on_actionWindowClose_triggered();
        void on_actionWindowNext_triggered();
        void on_actionWindowPrev_triggered();
        void on_actionOpen_triggered();
        void on_actionHelp_triggered();
        void on_actionExit_triggered();
        void on_actionSettings_triggered();
        void on_actionAbout_triggered();
        void on_actionReportBug_triggered();
        void on_actionSendFeedback_triggered();
        void on_actionRequestFeature_triggered();
        void on_actionRequestLicense_triggered();
        void actionWindowToggleView_triggered();
        void actionWindowFocus_triggered();
        void timerWelcome_timeout();
        void timerRefresh_timeout();
        void displayNote(const app::Event& event);
        void updateMenu(MainWidget* widget);

    private:
        Ui::MainWindow ui_;

    private:
        std::vector<MainModule*> modules_;
        std::vector<MainWidget*> widgets_;
        std::vector<QAction*> actions_;
        MainWidget* current_;                
    private:
        QStringList recents_;
        QString recent_save_nzb_path_;
        QString recent_load_nzb_path_;
        QTimer  refresh_timer_;
    private:
        app::Settings& settings_;
    };
    
    extern MainWindow* g_win;

} // gui


