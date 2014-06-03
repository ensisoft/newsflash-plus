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
#  include <QList>
#include <newsflash/warnpop.h>

#include <newsflash/sdk/uicomponent.h>

#include <memory>
#include "ui_mainwindow.h"

class QIcon;
class QCloseEvent;

namespace app {
    class valuestore;
    class mainapp;
}

namespace gui
{
    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(app::mainapp& app);
       ~MainWindow();

        void configure(const app::valuestore& values);
        void persist(app::valuestore& values);

        using QMainWindow::show;

        void compose(sdk::uicomponent* ui);
        void show(sdk::uicomponent* ui);
        void hide(sdk::uicomponent* ui);        
        void focus(sdk::uicomponent* ui);
        void refresh(sdk::uicomponent* ui);

    private:
        void closeEvent(QCloseEvent* event);

    private slots:
        void on_mainTab_currentChanged(int index);
        void on_mainTab_tabCloseRequested(int index);

    private:
        Ui::MainWindow ui_;
        app::mainapp& app_;
        sdk::uicomponent* current_;
        QList<sdk::uicomponent*> tabs_;
    };

} // gui
