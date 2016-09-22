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

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QtGui/QMessageBox>
#include "newsflash/warnpop.h"
#include <algorithm>
#include "dlgimport.h"
#include "dlgnewznab.h"
#include "app/newznab.h"
#include "app/debug.h"
#include "app/platform.h"

namespace gui
{

DlgImport::DlgImport(QWidget* parent, std::vector<app::Newznab::Account>& accs) : QDialog(parent), m_accounts(accs), m_query(nullptr)
{
    m_ui.setupUi(this);
    m_ui.progressBar->setVisible(true);
    m_ui.progressBar->setMinimum(0);
    m_ui.progressBar->setMaximum(0);
    m_ui.btnStart->setEnabled(false);

    auto callback = [=](const app::Newznab::ImportList& list) {
        m_query = nullptr;
        m_ui.progressBar->setVisible(false);
        m_ui.grpList->setTitle("Import List");        
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
            m_accounts.push_back(acc);
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(host);
            item->setIcon(QIcon("icons:ico_bullet_grey.png"));
            m_ui.listWidget->addItem(item);
        }
        m_ui.btnStart->setEnabled(true);
    };

    // begin importing the list from our host.
    m_query = app::Newznab::importList(std::move(callback));
}

DlgImport::~DlgImport()
{
    if (m_query)
        m_query->abort();
}

void DlgImport::on_btnStart_clicked()
{
    const auto email = m_ui.editEmail->text();
    if (email.isEmpty())
    {
        m_ui.editEmail->setFocus();
        return;
    }

    m_ui.btnStart->setEnabled(false);

    registerNext(email, 0);
}

void DlgImport::on_btnClose_clicked()
{
    close();
}

void DlgImport::registerNext(QString email, std::size_t index)
{
    if (index == m_accounts.size())
    {
        m_ui.btnStart->setEnabled(true);
        return;
    }

    auto& account = m_accounts[index];
    if (!account.apikey.isEmpty() ||
        !account.username.isEmpty())
        registerNext(email, index + 1);

    account.email = email;

    m_query = app::Newznab::apiRegisterUser(account, 
        std::bind(&DlgImport::registerInfo, this, std::placeholders::_1, 
            email, index));

    m_ui.log->appendPlainText(tr("Registering to %1").arg(account.apiurl));
    m_ui.progressBar->setVisible(true);
    m_ui.btnStart->setEnabled(false);

    auto* item = m_ui.listWidget->item(index);
    item->setIcon(QIcon("icons:ico_bullet_blue.png"));
    m_ui.listWidget->editItem(item);
}

void DlgImport::registerInfo(const app::Newznab::HostInfo& ret, QString email, std::size_t index)
{
    m_query = nullptr;

    m_ui.progressBar->setVisible(false);

    auto* item = m_ui.listWidget->item(index);
    if (ret.success)
    {
        m_ui.log->appendPlainText("Success!");
        m_ui.log->appendPlainText(QString("Username: %1").arg(ret.username));
        m_ui.log->appendPlainText(QString("Password: %1").arg(ret.password));
        m_ui.log->appendPlainText(QString("Apikey: %1").arg(ret.apikey));
        m_ui.log->appendPlainText("\n");            
        auto& account = m_accounts[index];
        account.username  = ret.username;
        account.password  = ret.password;
        account.apikey    = ret.apikey;

        if (account.apikey.isEmpty())
        {
            item->setIcon(QIcon("icons:ico_bullet_yellow.png"));            

            m_ui.log->appendPlainText("Manual login required.");

            QMessageBox::information(this, "Success",
                "The registration was succesful!\n"
                "However the sites requires a manual login to retrieve the API Key (under My Profile).\n"
                "Please login and Copy & Paste the key in order to complete registration. Thanks");

            QString url = account.apiurl;
            if (url.endsWith("api"))
                url.chop(3);
            app::openWeb(url);

            DlgNewznab dlg(this, account);
            if (dlg.exec() == QDialog::Rejected)
            {
                item->setIcon(QIcon("icons:ico_bullet_red.png"));
            }
            else
            {
                item->setIcon(QIcon("icons:ico_bullet_green.png"));
            }
        }
        else
        {
            item->setIcon(QIcon("icons:ico_bullet_green.png"));            
        }
    }
    else
    {
        m_ui.log->appendPlainText("Failed...");
        m_ui.log->appendPlainText(ret.error);
        m_ui.log->appendPlainText("\n");
        item->setIcon(QIcon("icons:ico_bullet_red.png"));
    }
    m_ui.listWidget->editItem(item);

    registerNext(email, index + 1);
}

} // gui
