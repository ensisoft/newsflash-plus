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

#include <newsflash/sdk/datastore.h>
#include "ui_mainwindow.h"
#include "../mainapp.h"

class QIcon;
class QAction;
class QCloseEvent;

namespace sdk {
    class widget;
} //

namespace gui
{
    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(app::mainapp& app);
       ~MainWindow();
       
        using QMainWindow::show;

   private:
        void show(const QString& name);
        void show(sdk::widget* widget);
        void hide(sdk::widget* widget);        
        void focus(sdk::widget* widget);    

    private:
        void closeEvent(QCloseEvent* event);

    private:
        void build_window_menu();
        bool savestate();
        void loadstate();
        void loadwidgets();

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
        void actionWindowToggle_triggered();
        void actionWindowFocus_triggered();

        void actionTray_activated(QSystemTrayIcon::ActivationReason);
        void timerWelcome_timeout();

    private:
        Ui::MainWindow ui_;

    private:
        app::mainapp& app_;        
        sdk::datastore settings_;
        sdk::widget* current_;        

        QList<sdk::widget*> tabs_;
        QList<QAction*> tabs_actions_;
        QSystemTrayIcon tray_;
    };

} // gui
