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
#  include "ui_downloads.h"
#include <newsflash/warnpop.h>
#include <memory>

#include "mainwidget.h"
#include "../tasklist.h"
#include "../connlist.h"

class QEvent;

namespace gui
{
    class downloads : public mainwidget
    {
        Q_OBJECT

    public:
        downloads();
       ~downloads();

        virtual void add_actions(QMenu& menu) override;
        virtual void add_actions(QToolBar& bar) override;
        virtual void loadstate(app::settings& s) override;
        virtual bool savestate(app::settings& s) override;
        virtual void refresh() override;
        virtual info information() const override 
        { return {"downloads.html", true, true}; }

    private slots:
        void on_actionConnect_triggered();
        void on_actionPreferSSL_triggered();
        void on_actionThrottle_triggered();
        void on_actionTaskPause_triggered();
        void on_actionTaskResume_triggered();
        void on_actionTaskMoveUp_triggered();
        void on_actionTaskMoveDown_triggered();
        void on_actionTaskMoveTop_triggered();
        void on_actionTaskMoveBottom_triggered();
        void on_actionTaskDelete_triggered();
        void on_actionTaskClear_triggered();
        void on_actionTaskOpenLog_triggered();
        void on_actionConnClone_triggered();
        void on_actionConnDelete_triggered();
        void on_actionConnRecycle_triggered();
        void on_actionConnOpenLog_triggered();
        void on_tableTasks_customContextMenuRequested(QPoint pos);
        void on_tableConns_customContextMenuRequested(QPoint pos);
        void on_tableConns_doubleClicked(const QModelIndex&);


    private:
        bool eventFilter(QObject* obj, QEvent* event);

    private:
        Ui::Downloads ui_;

    private:
        app::tasklist tasks_;
        app::connlist conns_;
        int panels_y_pos_;
    };

} // gui
