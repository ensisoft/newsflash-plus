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
#  include <QDialog>
#  include <QString>
#  include "ui_dlgaccount.h"
#include "newsflash/warnpop.h"
#include <memory>
#include "app/accounts.h"

namespace gui
{
    class DlgAccTest;

    class DlgAccount : public QDialog
    {
        Q_OBJECT

    public:
        DlgAccount(QWidget* parent, app::Accounts::Account& acc, bool isNew);
       ~DlgAccount();

    private:
        void changeEvent(QEvent *e);

    private slots:
        void on_btnTest_clicked();
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
    private:
        std::unique_ptr<DlgAccTest> test_;
    };

} // gui
