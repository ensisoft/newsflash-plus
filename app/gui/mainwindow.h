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
#  include <QtGui/QSystemTrayIcon>
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
    class settings;
} // app

namespace gui
{
    class mainwidget;
    class mainmodule;
    class DlgShutdown;

    // application's mainwindow. mainwindow manages the mainwidgets and
    // provides core GUI functionality that for example displaying common dialogs
    // for opening and saving application specific files (such as NZB).
    // it features a TabWidget that hosts between the mainwidgets.
    // Actions such as shutdown, drag&drop etc are handled initially
    // in the mainwindow and then propagated down to mainwidgets for handling.
    class mainwindow : public QMainWindow
    {
        Q_OBJECT

    public:
        using QMainWindow::show;

        // ctor/dtor
        mainwindow(app::settings& settings);
       ~mainwindow();
       
        // attach a new widget to the mainwindow and display it.
        // ownership of the object remains with the caller.
        void attach(mainwidget* widget, bool loadstate = false);

        // attach a new module to the mainwindow.
        // ownership of the module remains with the caller.
        void attach(mainmodule* module, bool loadstate = false);

        // update the widget's display state
        void update(mainwidget* widget);
    
        // detach all widgets
        void detach_all_widgets();

        // load previously stored application/gui state.
        void loadstate();

        // change application focus to the widget with the matching name      
        void show_widget(const QString& name);

        // open and show the application settings and select the
        // matching settings page.
        void show_setting(const QString& name);

        // show message in the window's message area
        void show_message(const QString& message);

        void prepare_file_menu();
        void prepare_main_tab();

        // perform a common action and select a download folder for some content.
        QString select_download_folder();

        // select a folder for storing a nzb file into.
        QString select_save_nzb_folder();

        // select a NZB file for opening.
        QString select_nzb_file(); 
        
        QString select_nzb_save_file(const QString& filename);

        QStringList get_recent_paths() const;

        quint32 choose_account(const QString& description);

   private:
        void show(const QString& name);
        void show(mainwidget* widget);
        void show(std::size_t index);
        void hide(mainwidget* widget);        
        void hide(std::size_t index);
        void focus(mainwidget* widget);    
        void build_window_menu();
        bool savestate(DlgShutdown* dlg);
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
        void on_actionHelp_triggered();
        void on_actionMinimize_triggered();
        void on_actionRestore_triggered();        
        void on_actionExit_triggered();
        void on_actionSettings_triggered();
        void on_actionAbout_triggered();
        void on_actionReportBug_triggered();
        void on_actionSendFeedback_triggered();
        void on_actionRequestFeature_triggered();
        void on_actionRequestLicense_triggered();
        void actionWindowToggleView_triggered();
        void actionWindowFocus_triggered();
        void actionTray_activated(QSystemTrayIcon::ActivationReason);
        void timerWelcome_timeout();
        void timerRefresh_timeout();

    private:
        Ui::MainWindow ui_;

    private:
        std::vector<mainmodule*> modules_;
        std::vector<mainwidget*> widgets_;
        std::vector<QAction*> actions_;
        mainwidget* current_;                
    private:
        QSystemTrayIcon tray_;
        QStringList recents_;
        QString recent_save_nzb_path_;
        QString recent_load_nzb_path_;
        QTimer  refresh_timer_;
    private:
        app::settings& settings_;
    };
    
    extern mainwindow* g_win;

} // gui


