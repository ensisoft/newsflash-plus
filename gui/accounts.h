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
#  include "ui_accounts.h"
#include "newsflash/warnpop.h"

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
        virtual void saveState(app::Settings& s) override;
        virtual void updateRegistration(bool success) override;

    private:
        bool eventFilter(QObject* object, QEvent* event);

    private slots:
        void on_actionAdd_triggered();
        void on_actionDel_triggered();
        void on_actionEdit_triggered();
        void on_btnResetMonth_clicked();
        void on_btnResetAllTime_clicked();
        void on_btnMonthlyQuota_toggled(bool checked);
        void on_btnFixedQuota_toggled(bool checked);
        void on_spinThisMonth_valueChanged(double value);
        void on_spinAllTime_valueChanged(double value);
        void on_spinQuotaAvail_valueChanged(double value);
        void on_spinQuotaUsed_valueChanged(double value);
        void on_listView_doubleClicked(const QModelIndex& index);
        void on_listView_customContextMenuRequested(QPoint pos);
        void on_lblRegister_linkActivated(QString);
        void on_grpQuota_toggled(bool on);
        void currentRowChanged();
        void updatePie();        

    private:
        Ui::Accounts ui_;
    private:
        bool in_row_changed_;
    };

} // gui