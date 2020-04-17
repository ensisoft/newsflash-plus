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
#  include <QMessageBox>
#include "newsflash/warnpop.h"

#include <algorithm>

#include "dlgimport.h"
#include "dlgnewznab.h"
#include "app/newznab.h"
#include "app/debug.h"
#include "app/platform.h"

namespace gui
{

DlgImport::DlgImport(QWidget* parent, std::vector<app::newznab::Account>& accs) : QDialog(parent), m_added_accounts(accs)
{
    m_ui.setupUi(this);
    m_ui.progressBar->setVisible(true);
    m_ui.progressBar->setMinimum(0);
    m_ui.progressBar->setMaximum(0);
    m_ui.btnStart->setEnabled(false);
    m_ui.btnClose->setEnabled(false);

    auto callback = [=](const app::newznab::ImportList& list) {
        m_query = nullptr;
        m_ui.progressBar->setVisible(false);
        m_ui.grpList->setTitle("Import Selected Servers");
        if (!list.success)
        {
            QMessageBox::critical(this, tr("Import List"),
                tr("There was an error import list of servers.\n%1").arg(list.error));
            m_ui.btnClose->setEnabled(true);
            return;
        }
        for (const auto& host : list.hosts)
        {
            app::newznab::Account acc;
            acc.apiurl = host;
            m_avail_accounts.push_back(acc);
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(host);
            item->setIcon(QIcon("icons:ico_bullet_grey.png"));
            m_ui.listWidget->addItem(item);
        }
        m_ui.btnStart->setEnabled(true);
        m_ui.btnClose->setEnabled(true);
    };

    // begin importing the list from our host.
    m_query = app::newznab::importServerList(std::move(callback));
}

DlgImport::~DlgImport()
{
    if (m_query)
        m_query->abort();
}

void DlgImport::on_btnStart_clicked()
{
    const bool try_to_register = m_ui.grpEmail->isChecked();
    if (try_to_register)
    {
        const auto& email = m_ui.editEmail->text();
        if (email.isEmpty())
        {
            m_ui.editEmail->setFocus();
            return;
        }
        registerNext(email, 0);
    }
    else
    {
        m_added_accounts.clear();

        for (size_t i=0; i<m_avail_accounts.size(); ++i)
        {
            QListWidgetItem* item = m_ui.listWidget->item(i);
            if (!item->isSelected())
                continue;

            m_added_accounts.push_back(m_avail_accounts[i]);
        }
        close();
    }
}

void DlgImport::on_btnClose_clicked()
{
    close();
}

void DlgImport::registerNext(const QString& email, std::size_t start_index)
{
    for (size_t i=start_index; i<m_avail_accounts.size(); ++i)
    {
        // this already has details.
        auto& account = m_avail_accounts[i];
        if (!account.apikey.isEmpty() || !account.username.isEmpty())
            continue;

        // is it selected?
        QListWidgetItem* item = m_ui.listWidget->item(i);
        if (!item->isSelected())
            continue;

        account.email = email;

        m_query = app::newznab::registerAccount(account,
            std::bind(&DlgImport::registerResultCallback, this, std::placeholders::_1,
                email, i));

        item->setIcon(QIcon("icons:ico_bullet_blue.png"));

        m_ui.log->appendPlainText(tr("Registering to %1").arg(account.apiurl));
        m_ui.listWidget->editItem(item);
        m_ui.progressBar->setVisible(true);
        m_ui.btnStart->setEnabled(false);
        m_ui.btnClose->setEnabled(false);
        return;
    }

    // no more accounts to try to register to.
    m_ui.btnStart->setEnabled(true);
    m_ui.btnClose->setEnabled(true);
    m_ui.progressBar->setVisible(false);
}

void DlgImport::registerResultCallback(const app::newznab::HostInfo& ret, QString email, std::size_t index)
{
    m_query = nullptr;

    auto* item = m_ui.listWidget->item(index);
    if (ret.success)
    {
        m_ui.log->appendPlainText("Success!");
        m_ui.log->appendPlainText(QString("Username: %1").arg(ret.username));
        m_ui.log->appendPlainText(QString("Password: %1").arg(ret.password));
        m_ui.log->appendPlainText(QString("Apikey: %1").arg(ret.apikey));
        m_ui.log->appendPlainText(QString("UserId: %1").arg(ret.userid));
        m_ui.log->appendPlainText("\n");
        auto& account     = m_avail_accounts[index];
        account.username  = ret.username;
        account.password  = ret.password;
        account.apikey    = ret.apikey;
        account.userid    = ret.userid;
        account.email     = email;

        if (account.apikey.isEmpty() || account.userid.isEmpty())
        {
            item->setIcon(QIcon("icons:ico_bullet_yellow.png"));

            m_ui.log->appendPlainText("Manual login required.");

            QMessageBox::information(this, "Success",
                "The registration was succesful!\n"
                "However the site requires a manual login to retrieve the API Key/UserId.\n\n"
                "Api key you can find under API menu.\n\n"
                "Look for: ?apikey=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                "The xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx part is your apikey.\n\n"
                "The User Id is needed for the RSS stream. You can find this under RSS Feeds\n"
                "Look for: &i=yyyyy&r=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"
                "The yyyyy part is your User Id.\n\n"
                "Please login and Copy & Paste the keys in order to complete registration. Thanks");

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
        m_added_accounts.push_back(account);
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
