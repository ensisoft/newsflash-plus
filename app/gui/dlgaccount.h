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
#  include <QtGui/QDialog>
#  include <QString>
#  include "ui_dlgaccount.h"
#include <newsflash/warnpop.h>
#include "../accounts.h"

namespace gui
{
    class DlgAccount : public QDialog 
    {
        Q_OBJECT

    public:
        DlgAccount(QWidget* parent, app::Accounts::Account& acc, bool isNew);
       ~DlgAccount();
        
    private:
        void changeEvent(QEvent *e);

    private slots:
        void on_btnAccept_clicked();
        void on_btnBrowse_clicked();        
        void on_btnCancel_clicked();
        void on_grpSecure_clicked(bool val);
        void on_grpGeneral_clicked(bool val);
        void on_edtName_textEdited();

    private:
        Ui::DlgAccount ui_;

    private:
        app::Accounts::Account& acc_;
        bool isNew_;
        QString name_;
        QString path_;
    };

} // gui
