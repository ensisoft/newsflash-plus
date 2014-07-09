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
#include <newsflash/sdk/window.h>
#include <newsflash/sdk/model.h>
#include "ui_mainwindow.h"
#include "../mainapp.h"

#include <vector>
#include <memory>

class QIcon;
class QAction;
class QCloseEvent;

namespace sdk {
    class widget;
} //

namespace gui
{
    class MainWindow : public QMainWindow, public sdk::window
    {
        Q_OBJECT

    public:
        MainWindow(app::mainapp& app);
       ~MainWindow();
       
        using QMainWindow::show;

        virtual sdk::model* create_model(const char* klazz) override;

        virtual void show_widget(const QString& name) override;

        virtual void show_setting(const QString& name) override;

        virtual QString select_download_folder() override;

        virtual QString select_save_nzb_folder() override;

        virtual void recents(QStringList& paths) const override;

   private:
        void show(const QString& name);
        void show(sdk::widget* widget);
        void show(std::size_t index);
        void hide(sdk::widget* widget);        
        void hide(std::size_t index);
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
        void on_actionSettings_triggered();
        void actionWindowToggleView_triggered();
        void actionWindowFocus_triggered();

        void actionTray_activated(QSystemTrayIcon::ActivationReason);
        void timerWelcome_timeout();

    private:
        Ui::MainWindow ui_;

    private:
        app::mainapp& app_;        
        sdk::datastore settings_;
        sdk::widget* current_;        
        std::vector<std::unique_ptr<sdk::widget>> widgets_;
        std::vector<QAction*> actions_;
    private:
        QSystemTrayIcon tray_;
        QStringList recents_;
        QString recent_save_nzb_path_;
        QString recent_load_nzb_path_;
    };

} // gui
