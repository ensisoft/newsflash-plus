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

#include <newsflash/sdk/widget.h>
#include <newsflash/sdk/model.h>
#include <newsflash/sdk/message.h>
#include <newsflash/sdk/message_account.h>
#include <newsflash/warnpush.h>
#include <newsflash/warnpop.h>
#include <memory>
#include "message.h"
#include "ui_accounts.h"

class QMovie;
class QAbstractItemModel;

namespace gui
{
    class Accounts : public sdk::widget
    {
        Q_OBJECT
        
    public:
        Accounts(sdk::model& model);
       ~Accounts();

        virtual void add_actions(QMenu& menu) override;
        virtual void add_actions(QToolBar& bar) override;
        virtual info information() const override;
        virtual void load(const sdk::datastore& data) override;
        virtual void save(sdk::datastore& data) override;

        void on_message(const char* sender, msg_first_launch& msg);
        void on_message(const char* sender, sdk::msg_account_downloads_update& msg);
        void on_message(const char* sender, sdk::msg_account_quota_update& msg);

    private:
        void advertise(bool show);        
        
    private:
        bool eventFilter(QObject* object, QEvent* event);

    private slots:
        void on_actionNew_triggered();
        void on_actionDel_triggered();
        void on_actionProperties_triggered();
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
        sdk::model& model_;
        std::unique_ptr<QMovie> movie_;
        QString license_;
        bool in_row_changed_;
    };

} // gui