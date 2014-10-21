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

#include "dlgchoose.h"

namespace gui
{

DlgChoose::DlgChoose(QWidget* parent, const QStringList& accounts, const QString& task) : QDialog(parent)
{
    ui_.setupUi(this);

    for (int i=0; i<accounts.size(); ++i)
        ui_.cmbList->addItem(accounts[i]);

    ui_.lblHint->setText(task);
}

DlgChoose::~DlgChoose()
{}

QString DlgChoose::account() const 
{ return ui_.cmbList->currentText(); }

bool DlgChoose::remember() const 
{ return ui_.chkRemember->isChecked(); }

void DlgChoose::on_btnAccept_clicked()
{
    accept();
}

void DlgChoose::on_btnCancel_clicked()
{
    reject();
}

void DlgChoose::changeEvent(QEvent* e)
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


} // gui