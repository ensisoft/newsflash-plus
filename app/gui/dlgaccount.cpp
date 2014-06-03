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

namespace gui
{

DlgAccount::DlgAccount(QWidget* parent, app::accounts::account& account, bool create) : QDialog(parent), 
    account_(account), create_(create)
{
    ui_.setupUi(this);

    ui_.edtName->setText(account.name);
    ui_.edtHost->setText(account.general.host);
    ui_.edtPort->setText(QString::number(account.general.port));
    ui_.edtHostSecure->setText(account.secure.host);
    ui_.edtPortSecure->setText(QString::number(account.secure.port));
    ui_.edtUsername->setText(account.user);
    ui_.edtPassword->setText(account.pass);
    ui_.edtDataFiles->setCursorPosition(0);
    ui_.chkCompressedHeaders->setChecked(account.enable_compression);
    ui_.chkBatchMode->setChecked(account.enable_pipelining);
    ui_.grpSecure->setChecked(account.secure.enabled);
    ui_.grpNonSecure->setChecked(account.general.enabled);
    ui_.grpLogin->setChecked(account.requires_login);
    ui_.btnOK->setEnabled(account.general.enabled ||
        account.secure.enabled);
    ui_.maxConnections->setValue(account.maxconn);

    ui_.edtName->setFocus();
    ui_.edtName->setSelection(0, account.name.size());

    if (create)
        ui_.edtDataFiles->setText(
            QDir::toNativeSeparators(account.path + "/" + account.name));
    else ui_.edtDataFiles->setText(account.path);
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

void DlgAccount::on_btnOK_clicked()
{
    // validate and accept input if valid
    account_.name               = ui_.edtName->text();
    account_.general.enabled    = ui_.grpNonSecure->isChecked();
    account_.secure.enabled     = ui_.grpSecure->isChecked();
    account_.requires_login     = ui_.grpLogin->isCheckable();    
    account_.general.port       = ui_.edtPort->text().toInt();
    account_.general.host       = ui_.edtHost->text();            
    account_.secure.port        = ui_.edtPortSecure->text().toInt();
    account_.secure.host        = ui_.edtHostSecure->text();            
    account_.user               = ui_.edtUsername->text();
    account_.pass               = ui_.edtPassword->text();
    account_.requires_login     = ui_.grpLogin->isChecked();
    account_.path               = ui_.edtDataFiles->text();
    account_.maxconn            = ui_.maxConnections->value();
    account_.enable_compression = ui_.chkCompressedHeaders->isChecked();
    account_.enable_pipelining  = ui_.chkBatchMode->isChecked();

    if (account_.name.isEmpty())
    {
        ui_.edtName->setFocus();
        return;
    }
    if (account_.general.enabled)
    {
        if (account_.general.port <= 0)
        {
            ui_.edtPort->setFocus();
            return;
        }
        if (account_.general.host.isEmpty())
        {
            ui_.edtHost->setFocus();
            return;
        }
    }
    if (account_.secure.enabled)
    {
        if (account_.secure.port <= 0)
        {
            ui_.edtPort->setFocus();
            return;
        }
        if (account_.secure.host.isEmpty())
        {
            ui_.edtHost->setFocus();
            return;
        }
    }
    if (account_.requires_login)
    {
        if (account_.user.isEmpty())
        {
            ui_.edtUsername->setFocus();
            return;
        }
        if (account_.pass.isEmpty())
        {
            ui_.edtPassword->setFocus();
            return;
        }
    }
    if (account_.path.isEmpty())
    {
        ui_.edtDataFiles->setFocus();
        return;
    }

    if (account_.maxconn < 0)
    {
        ui_.maxConnections->setFocus();
        return;
    }

    accept();
}

void DlgAccount::on_btnBrowseData_clicked()
{
    const auto& dir = QFileDialog::getExistingDirectory(
        this, tr("Location for data files"));
    if (dir.isEmpty())
        return;

    QString path;
    if (create_)
    {
        const auto& name = ui_.edtName->text();
        path = QDir::toNativeSeparators(dir + "/" + name);
    }
    else 
    {
        path = QDir::toNativeSeparators(dir);
    }
    ui_.edtDataFiles->setText(path);
}

void DlgAccount::on_grpSecure_clicked(bool val)
{
    // need to enable at least 1 server
    ui_.btnOK->setEnabled(
        ui_.grpSecure->isChecked() || 
        ui_.grpNonSecure->isChecked());
}

void DlgAccount::on_grpNonSecure_clicked(bool val)
{
    // need to enable at least 1 server
    ui_.btnOK->setEnabled(
        ui_.grpSecure->isChecked() || 
        ui_.grpNonSecure->isChecked());    
}

void DlgAccount::on_edtName_textEdited()
{
    if (!create_)
        return;

    // modify the default data file path on the fly 
    const auto& name = ui_.edtName->text();
    const auto& path = QDir::toNativeSeparators(account_.path + "/" + name);
    ui_.edtDataFiles->setText(path);
}

} // gui