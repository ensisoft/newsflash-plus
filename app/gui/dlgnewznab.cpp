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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMessageBox>
#include <newsflash/warnpop.h>
#include "dlgnewznab.h"

namespace gui
{

DlgNewznab::DlgNewznab(QWidget* parent, app::Newznab::Account& acc) : QDialog(parent), acc_(acc), query_(nullptr)
{
    ui_.setupUi(this);
    if (!acc.apikey.isEmpty())
        ui_.editKey->setText(acc.apikey);

    ui_.editHost->setText(acc.apiurl);
    ui_.editEmail->setText(acc.email);
    ui_.editPassword->setText(acc.password);
    ui_.editUsername->setText(acc.username);
    ui_.progressBar->setVisible(false);
    ui_.progressBar->setMaximum(0);
    ui_.progressBar->setMinimum(0);

    #define HIDE_IF_EMPTY(e, l) \
        if (e->text().isEmpty()) {\
            e->setVisible(false); \
            l->setVisible(false); \
        }

    HIDE_IF_EMPTY(ui_.editEmail, ui_.lblEmail);
    HIDE_IF_EMPTY(ui_.editPassword, ui_.lblPassword);
    HIDE_IF_EMPTY(ui_.editUsername, ui_.lblUsername);

    #undef HIDE_IF_EMPTY

    updateGeometry();
}

DlgNewznab::~DlgNewznab()
{
    if (query_)
        query_->abort();
}

void DlgNewznab::on_btnAccept_clicked()
{
    acc_.email    = ui_.editEmail->text();
    acc_.apikey   = ui_.editKey->text();
    acc_.apiurl   = ui_.editHost->text();
    acc_.password = ui_.editPassword->text();
    acc_.username = ui_.editUsername->text();

    accept();
}

void DlgNewznab::on_btnCancel_clicked()
{
    reject();
}

void DlgNewznab::on_btnTest_clicked()
{
    app::Newznab::Account acc;
    acc.email    = ui_.editEmail->text();
    acc.apikey   = ui_.editKey->text();
    acc.apiurl   = ui_.editHost->text();
    acc.password = ui_.editPassword->text();
    acc.username = ui_.editUsername->text();

    if (acc.apiurl.isEmpty())
    {
        ui_.editHost->setFocus();
        return;
    }
    if (acc.apikey.isEmpty())
    {
        ui_.editKey->setFocus();
        return;
    }


    auto callback = [=](const app::Newznab::HostInfo& info) {
        query_ = nullptr;
        ui_.progressBar->setVisible(false);
        ui_.btnTest->setEnabled(true);
        if (info.success)
        {
            QMessageBox::information(this, acc.apiurl, 
                QString("Success! Server reports:\n"
                    "Strapline: %1\n"
                    "Version: %2\n"
                    "Contact: %3\n")
                .arg(info.strapline)
                .arg(info.version)
                .arg(info.email));
        }
        else
        {
            QMessageBox::critical(this, acc.apiurl,
                info.error);
        }
    };

    query_ = app::Newznab::apiTest(acc, callback);

    ui_.progressBar->setVisible(true);
    ui_.btnTest->setEnabled(false);
}

} // gui