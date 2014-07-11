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
#  include <QtGui/QAction>
#  include <QList>
#include <newsflash/warnpop.h>

//#include <newsflash/sdk/model.h>
#include "ui_mainwindow.h"
#include "../mainapp.h"
#include "../datastore.h"

#include <vector>
#include <memory>

class QIcon;
class QAction;
class QCloseEvent;
class QCoreApplication;

namespace gui
{
    class mainwidget;

    class mainwindow : public QMainWindow
    {
        Q_OBJECT

    public:
        using QMainWindow::show;

        mainwindow(app::mainapp& app);
       ~mainwindow();
       
        void attach(mainwidget* widget);

        void loadstate();

        void load(const app::datastore& store);
        void save(app::datastore& store);

        void show_widget(const QString& name);

        void show_setting(const QString& name);

        QString select_download_folder();

        QString select_save_nzb_folder();
        
        void recents(QStringList& paths) const;

        void update(mainwidget* widget);

   private:
        void show(const QString& name);
        void show(mainwidget* widget);
        void show(std::size_t index);
        void hide(mainwidget* widget);        
        void hide(std::size_t index);
        void focus(mainwidget* widget);    

    private:
        void closeEvent(QCloseEvent* event);

    private:
        void build_window_menu();
        bool savestate();
        
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
        void on_actionOpenNZB_triggered();
        void actionWindowToggleView_triggered();
        void actionWindowFocus_triggered();

        void actionTray_activated(QSystemTrayIcon::ActivationReason);
        void timerWelcome_timeout();

    private:
        Ui::MainWindow ui_;

    private:
        app::mainapp& app_;        
        app::datastore settings_;
        std::vector<mainwidget*> widgets_;
        std::vector<QAction*> actions_;
        std::vector<std::unique_ptr<mainwidget>> extras_;
        mainwidget* current_;                
    private:
        QSystemTrayIcon tray_;
        QStringList recents_;
        QString recent_save_nzb_path_;
        QString recent_load_nzb_path_;
    };

    extern mainwindow* g_win;

} // gui
