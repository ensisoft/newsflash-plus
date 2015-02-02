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
#  include "ui_accounts.h"
#include <newsflash/warnpop.h>

#include "mainwidget.h"

namespace gui
{
    // accounts GUI module
    class Accounts : public MainWidget
    {
        Q_OBJECT
        
    public:
        Accounts();
       ~Accounts();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual info getInformation() const override;
        virtual void loadState(app::Settings& s) override;
        virtual bool saveState(app::Settings& s) override;
        virtual void firstLaunch(bool add_account) override;

    private:
        bool eventFilter(QObject* object, QEvent* event);

    private:
        void updatePie();

    private slots:
        void on_actionAdd_triggered();
        void on_actionDel_triggered();
        void on_actionEdit_triggered();
        void on_btnResetMonth_clicked();
        void on_btnResetAllTime_clicked();
        void on_btnMonthlyQuota_toggled(bool checked);
        void on_btnFixedQuota_toggled(bool checked);
        void on_spinTotal_valueChanged(double value);
        void on_spinSpent_valueChanged(double value);
        void on_listView_doubleClicked(const QModelIndex& index);
        void on_listView_customContextMenuRequested(QPoint pos);
        void on_grpQuota_toggled(bool on);
        void currentRowChanged();

    private:
        Ui::Accounts ui_;
    private:
        bool in_row_changed_;
    };

} // gui