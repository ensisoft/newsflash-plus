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
#include "command.h"

namespace gui
{

DlgAccount::DlgAccount(QWidget* parent, int fetch) : QDialog(parent)
{
    ui_.setupUi(this);

    // const auto cmd = gui::fetch<cmd_get_account>(fetch);

    // acc_id_ = cmd->id;

    // ui_.edtName->setText(cmd->name);
    // ui_.edtHost->setText(cmd->general_host);
    // ui_.edtPort->setText(QString::number(cmd->general_port));
    // ui_.edtHostSecure->setText(cmd->secure_host);
    // ui_.edtPortSecure->setText(QString::number(cmd->secure_port));
    // ui_.edtUsername->setText(cmd->username);
    // ui_.edtPassword->setText(cmd->password);
    // ui_.chkCompression->setChecked(cmd->enable_compression);
    // ui_.chkPipelining->setChecked(cmd->enable_pipelining);
    // ui_.grpSecure->setChecked(cmd->enable_secure_server);
    // ui_.grpGeneral->setChecked(cmd->enable_general_server);
    // ui_.grpLogin->setChecked(cmd->enable_login);
}

DlgAccount::DlgAccount(QWidget* parent) : QDialog(parent)
{
    ui_.setupUi(this);

    // const auto cmd = gui::fetch<cmd_new_account>();

    // acc_id_ = cmd->id;

    // ui_.edtName->setText(cmd->name);
    // ui_.edtName->setFocus();
    // ui_.edtName->setSelection(0, cmd->name.size());    
    // ui_.edtPort->setText(QString::number(cmd->general_port));
    // ui_.edtPortSecure->setText(QString::number(cmd->secure_port));
    // ui_.chkCompression->setChecked(cmd->enable_compression);
    // ui_.chkPipelining->setChecked(cmd->enable_pipelining);
    // ui_.grpSecure->setChecked(false);
    // ui_.grpGeneral->setChecked(false);
    // ui_.grpLogin->setChecked(false);
    // ui_.btnOK->setEnabled(false);
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
    // auto cmd = std::make_shared<cmd_set_account>();
    // cmd->id                    = acc_id_;
    // cmd->name                  = ui_.edtName->text();
    // cmd->enable_general_server = ui_.grpGeneral->isChecked();
    // cmd->enable_secure_server  = ui_.grpSecure->isChecked();
    // cmd->enable_login          = ui_.grpLogin->isChecked();
    // cmd->general_port          = ui_.edtPort->text().toInt();
    // cmd->general_host          = ui_.edtHost->text();            
    // cmd->secure_port           = ui_.edtPortSecure->text().toInt();
    // cmd->secure_host           = ui_.edtHostSecure->text();            
    // cmd->username              = ui_.edtUsername->text();
    // cmd->password              = ui_.edtPassword->text();
    // cmd->connections           = ui_.maxConnections->value();
    // cmd->enable_compression    = ui_.chkCompression->isChecked();
    // cmd->enable_pipelining     = ui_.chkPipelining->isChecked();

    // if (cmd->name.isEmpty())
    // {
    //     ui_.edtName->setFocus();
    //     return;
    // }
    // if (cmd->enable_general_server)
    // {
    //     if (cmd->general_port <= 0)
    //     {
    //         ui_.edtPort->setFocus();
    //         return;
    //     }
    //     if (cmd->general_host.isEmpty())
    //     {
    //         ui_.edtHost->setFocus();
    //         return;
    //     }
    // }
    // if (cmd->enable_secure_server)
    // {
    //     if (cmd->secure_port <= 0)
    //     {
    //         ui_.edtPort->setFocus();
    //         return;
    //     }
    //     if (cmd->secure_host.isEmpty())
    //     {
    //         ui_.edtHost->setFocus();
    //         return;
    //     }
    // }
    // if (cmd->enable_login)
    // {
    //     if (cmd->username.isEmpty())
    //     {
    //         ui_.edtUsername->setFocus();
    //         return;
    //     }
    //     if (cmd->password.isEmpty())
    //     {
    //         ui_.edtPassword->setFocus();
    //         return;
    //     }
    // }
    // if (cmd->connections < 0)
    // {
    //     ui_.maxConnections->setFocus();
    //     return;
    // }
//    if (send(cmd) == command::status::accept)
//        accept();
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