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
#include <algorithm>
#include "dlgimport.h"
#include "../newznab.h"
#include "../debug.h"

namespace gui
{

DlgImport::DlgImport(QWidget* parent, std::vector<app::Newznab::Account>& accs) : QDialog(parent), accs_(accs), query_(nullptr)
{
    ui_.setupUi(this);
    ui_.progressBar->setVisible(true);
    ui_.progressBar->setMinimum(0);
    ui_.progressBar->setMaximum(0);

    auto callback = [=](const app::Newznab::ImportList& list) {
        query_ = nullptr;
        ui_.progressBar->setVisible(false);
        ui_.grpList->setTitle("Import List");        
        if (!list.success) 
        {
            QMessageBox::critical(this, tr("Import List"),
                tr("There was an error import list of servers.\n%1").arg(list.error));
            return;
        }
        for (const auto& host : list.hosts)
        {
            app::Newznab::Account acc;
            acc.apiurl = host;
            accs_.push_back(acc);
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(host);
            item->setIcon(QIcon("icons:ico_bullet_grey.png"));
            ui_.listWidget->addItem(item);
        }
        ui_.btnStart->setEnabled(true);
    };

    // begin importing the list from our host.
    query_ = app::Newznab::importList(std::move(callback));

    // testing stuff here.
    // app::Newznab::Account acc;
    // acc.apiurl = "https://www.nzbsooti.sx/api";
    // acc.email  = "ensiferum_@hotmail.com";    
    // accs_.push_back(acc);

    // QListWidgetItem* item = new QListWidgetItem();
    // item->setText(acc.apiurl);
    // item->setIcon(QIcon("icons:ico_bullet_grey.png"));
    // ui_.listWidget->addItem(item);
}

DlgImport::~DlgImport()
{
    if (query_)
        query_->abort();
}

void DlgImport::on_btnStart_clicked()
{
    const auto email = ui_.editEmail->text();
    if (email.isEmpty())
    {
        ui_.editEmail->setFocus();
        return;
    }

    ui_.btnStart->setEnabled(false);

    registerNext(email);
}

void DlgImport::on_btnClose_clicked()
{
    close();
}

void DlgImport::registerNext(QString email)
{
    auto it = std::find_if(std::begin(accs_), std::end(accs_),
        [](const app::Newznab::Account& acc) {
            return acc.apikey.isEmpty();
        });
    if (it == std::end(accs_))
    {
        //ui_.btnStart->setEnabled(true);
        return;
    }

    auto callback = [=](const app::Newznab::HostInfo& info, QString apiurl) {
        query_ = nullptr;
        ui_.progressBar->setVisible(false);

        auto* item = ui_.listWidget->findItems(apiurl, Qt::MatchFixedString)[0];
        if (info.success)
        {
            ui_.log->appendPlainText("Success!");
            ui_.log->appendPlainText(QString("Username: %1").arg(info.username));
            ui_.log->appendPlainText(QString("Password: %1").arg(info.password));
            ui_.log->appendPlainText(QString("Apikey: %1").arg(info.apikey));
            ui_.log->appendPlainText("\n");            
            app::Newznab::Account acc;
            acc.email    = email;
            acc.apiurl   = apiurl;            
            acc.username = info.username;
            acc.password = info.password;
            acc.apikey   = info.apikey;
            accs_.push_back(acc);            
            item->setIcon(QIcon("icons:ico_bullet_green.png"));
        }
        else
        {
            ui_.log->appendPlainText("Failed...");
            ui_.log->appendPlainText(info.error);
            ui_.log->appendPlainText("\n");
            item->setIcon(QIcon("icons:ico_bullet_red.png"));
        }
        ui_.listWidget->editItem(item);
        registerNext(email);
    };
    const auto& acc = *it;

    query_ = app::Newznab::apiRegisterUser(acc, std::bind(std::move(callback),
        std::placeholders::_1, acc.apiurl));

    ui_.log->appendPlainText(tr("Registering to %1").arg(acc.apiurl));
    ui_.progressBar->setVisible(true);

    auto* item = ui_.listWidget->findItems(acc.apiurl, Qt::MatchFixedString)[0];
    item->setIcon(QIcon("icons:ico_bullet_blue.png"));
    ui_.listWidget->editItem(item);

    accs_.erase(it);

}

} // gui