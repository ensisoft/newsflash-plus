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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QFileDialog>
#  include <QDir>
#include <newsflash/warnpop.h>
#include "dlgaccount.h"
#include "../utility.h"
#include "../debug.h"

namespace gui
{

DlgAccount::DlgAccount(QWidget* parent, app::Accounts::Account& acc, bool isNew) : QDialog(parent), acc_(acc), isNew_(isNew)
{
    ui_.setupUi(this);

    ui_.edtName->setText(acc_.name);
    ui_.edtHost->setText(acc_.generalHost);
    ui_.edtPort->setText(QString::number(acc_.generalPort));
    ui_.edtHostSecure->setText(acc_.secureHost);
    ui_.edtPortSecure->setText(QString::number(acc_.securePort));
    ui_.edtUsername->setText(acc_.username);
    ui_.edtPassword->setText(acc_.password);
    ui_.chkCompression->setChecked(acc_.enableCompression);
    ui_.chkPipelining->setChecked(acc_.enablePipelining);
    ui_.grpSecure->setChecked(acc_.enableSecureServer);
    ui_.grpGeneral->setChecked(acc_.enableGeneralServer);
    ui_.grpLogin->setChecked(acc_.enableLogin);
    ui_.maxConnections->setValue(acc_.maxConnections);
    ui_.edtDataPath->setText(acc.datapath);
    ui_.edtDataPath->setText(acc.datapath);
    ui_.edtDataPath->setCursorPosition(0);

    ui_.edtName->setFocus();    

    ui_.btnAccept->setEnabled(
        ui_.grpSecure->isChecked() || 
        ui_.grpGeneral->isChecked());     

    if (isNew_)
    {
        name_ = acc_.name;
        path_ = QDir::cleanPath(acc_.datapath.remove(name_));
    }
}

DlgAccount::~DlgAccount()
{}

void DlgAccount::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui_.retranslateUi(this);
        break;
    default:
        break;
    }
}

void DlgAccount::on_btnAccept_clicked()
{
    acc_.name                = ui_.edtName->text();
    acc_.enableGeneralServer = ui_.grpGeneral->isChecked();
    acc_.enableSecureServer  = ui_.grpSecure->isChecked();
    acc_.enableLogin         = ui_.grpLogin->isChecked();
    acc_.generalPort         = ui_.edtPort->text().toInt();
    acc_.generalHost         = ui_.edtHost->text();
    acc_.securePort          = ui_.edtPortSecure->text().toInt();
    acc_.secureHost          = ui_.edtHostSecure->text();
    acc_.username            = ui_.edtUsername->text();
    acc_.password            = ui_.edtPassword->text();
    acc_.maxConnections      = ui_.maxConnections->value();
    acc_.enableCompression   = ui_.chkCompression->isChecked();
    acc_.enablePipelining    = ui_.chkPipelining->isChecked();
    acc_.datapath            = ui_.edtDataPath->text();

    if (acc_.name.isEmpty())
    {
        ui_.edtName->setFocus();
        return;
    }

    // must have either general or secure server enabled
    // otherwise the account is unusable!
    if (!acc_.enableGeneralServer && !acc_.enableSecureServer)
        return;

    if (acc_.enableGeneralServer)
    {
        if (acc_.generalPort <= 0)
        {
            ui_.edtPort->setFocus();
            return;
        }
        if (acc_.generalHost.isEmpty())
        {
            ui_.edtHost->setFocus();
            return;
        }
    }
    if (acc_.enableSecureServer)
    {
        if (acc_.securePort <= 0)
        {
            ui_.edtPort->setFocus();
            return;
        }
        if (acc_.secureHost.isEmpty())
        {
            ui_.edtHost->setFocus();
            return;
        }
    }
    if (acc_.enableLogin)
    {
        if (acc_.username.isEmpty())
        {
            ui_.edtUsername->setFocus();
            return;
        }
        if (acc_.password.isEmpty())
        {
            ui_.edtPassword->setFocus();
            return;
        }
    }
    if (acc_.maxConnections < 0)
    {
        ui_.maxConnections->setFocus();
        return;
    }
    if (acc_.datapath.isEmpty())
    {
        ui_.edtDataPath->setFocus();
        return;
    }
    
    accept();
}

void DlgAccount::on_btnBrowse_clicked()
{
    const auto dir = QFileDialog::getExistingDirectory(this, tr("Select Data Directory"));
    if (dir.isEmpty())
        return;

    ui_.edtDataPath->setText(dir);
    isNew_ = false;
    //ui_.edtDataPath->setCursorPosition(0);
}

void DlgAccount::on_btnCancel_clicked()
{
    reject();
}

void DlgAccount::on_grpSecure_clicked(bool val)
{
    // need to enable at least 1 server
    ui_.btnAccept->setEnabled(
        ui_.grpSecure->isChecked() || 
        ui_.grpGeneral->isChecked());
}

void DlgAccount::on_grpGeneral_clicked(bool val)
{
    // need to enable at least 1 server
    ui_.btnAccept->setEnabled(
        ui_.grpSecure->isChecked() || 
        ui_.grpGeneral->isChecked());    
}

void DlgAccount::on_edtName_textEdited()
{
    if (!isNew_)
        return;

    const auto name = ui_.edtName->text();
    const auto path = app::joinPath(path_, name);
    ui_.edtDataPath->setText(path);
    name_ = name;

    //DEBUG("New account name is %1", name_);
    //DEBUG("New account path is %2", path);
}

} // gui