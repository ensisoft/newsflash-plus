// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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
#  include "ui_repair.h"
#include <newsflash/warnpop.h>
#include "mainwidget.h"
#include "../repair.h"

namespace gui
{
    class Repair : public MainWidget
    {
        Q_OBJECT

    public:
        Repair();
       ~Repair();

        virtual void addActions(QToolBar& bar) override;
        virtual void addActions(QMenu& menu) override;
        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;

        virtual info getInformation() const override
        { return {"repair.html", true, true }; }

    private slots:
        void on_tableView_customContextMenuRequested(QPoint);
        void on_actionRecover_triggered();
        void on_actionStop_triggered();
        void on_actionAdd_triggered();
        void on_actionDel_triggered();
        void recoveryStart(const app::Recovery& rec);
        void recoveryReady(const app::Recovery& rec);
        void scanProgress(const QString& file, int val);
        void repairProgress(const QString& step, int done);

    private:
        Ui::Repair ui_;
    private:
    };
} // gui

