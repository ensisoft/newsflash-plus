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

#include "dlgaccount.h"

namespace gui
{

DlgAccount::DlgAccount(QWidget* parent, app::account& acc) : QDialog(parent), acc_(acc)
{
    ui_.setupUi(this);

    ui_.edtName->setText(acc_.name());
    ui_.edtHost->setText(acc_.general_host());

    ui_.edtPort->setText(QString::number(acc_.general_port()));
    ui_.edtHostSecure->setText(acc_.secure_host());
    ui_.edtPortSecure->setText(QString::number(acc_.secure_port()));
    ui_.edtUsername->setText(acc_.username());
    ui_.edtPassword->setText(acc_.password());
    ui_.chkCompression->setChecked(acc_.enable_compression());
    ui_.chkPipelining->setChecked(acc_.enable_pipelining());
    ui_.grpSecure->setChecked(acc_.enable_secure_server());
    ui_.grpGeneral->setChecked(acc_.enable_general_server());
    ui_.grpLogin->setChecked(acc_.enable_login());

    ui_.edtName->setFocus();    
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
    acc_.name(ui_.edtName->text());
    acc_.enable_general_server(ui_.grpGeneral->isChecked());
    acc_.enable_secure_server(ui_.grpSecure->isChecked());
    acc_.enable_login(ui_.grpLogin->isChecked());
    acc_.general_port(ui_.edtPort->text().toInt());
    acc_.general_host(ui_.edtHost->text());
    acc_.secure_port(ui_.edtPortSecure->text().toInt());
    acc_.secure_host(ui_.edtHostSecure->text());
    acc_.username(ui_.edtUsername->text());
    acc_.password(ui_.edtPassword->text());
    acc_.connections(ui_.maxConnections->value());
    acc_.enable_compression(ui_.chkCompression->isChecked());
    acc_.enable_pipelining(ui_.chkPipelining->isChecked());

    if (acc_.name().isEmpty())
    {
        ui_.edtName->setFocus();
        return;
    }
    if (acc_.enable_general_server())
    {
        if (acc_.general_port() <= 0)
        {
            ui_.edtPort->setFocus();
            return;
        }
        if (acc_.general_host().isEmpty())
        {
            ui_.edtHost->setFocus();
            return;
        }
    }
    if (acc_.enable_secure_server())
    {
        if (acc_.secure_port() <= 0)
        {
            ui_.edtPort->setFocus();
            return;
        }
        if (acc_.secure_host().isEmpty())
        {
            ui_.edtHost->setFocus();
            return;
        }
    }
    if (acc_.enable_login())
    {
        if (acc_.username().isEmpty())
        {
            ui_.edtUsername->setFocus();
            return;
        }
        if (acc_.password().isEmpty())
        {
            ui_.edtPassword->setFocus();
            return;
        }
    }
    if (acc_.connections() < 0)
    {
        ui_.maxConnections->setFocus();
        return;
    }
    
    accept();
}

void DlgAccount::on_grpSecure_clicked(bool val)
{
    // need to enable at least 1 server
    ui_.btnOK->setEnabled(
        ui_.grpSecure->isChecked() || 
        ui_.grpGeneral->isChecked());
}

void DlgAccount::on_grpGeneral_clicked(bool val)
{
    // need to enable at least 1 server
    ui_.btnOK->setEnabled(
        ui_.grpSecure->isChecked() || 
        ui_.grpGeneral->isChecked());    
}

} // gui