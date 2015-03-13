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

#define LOGTAG "search"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#include <newsflash/warnpop.h>
#include "searchmodule.h"
#include "search.h"
#include "dlgnewznab.h"
#include "dlgimport.h"
#include "../settings.h"
#include "../debug.h"
#include "../newznab.h"

namespace gui
{

SearchSettings::SearchSettings(std::vector<app::Newznab::Account> newznab) : newznab_(std::move(newznab))
{
    ui_.setupUi(this);
    ui_.btnDel->setEnabled(false);
    ui_.btnEdit->setEnabled(false);

    ui_.listServers->blockSignals(true);
    for (const auto& acc : newznab_)
    {
        auto* item = new QListWidgetItem();
        item->setIcon(QIcon("icons:ico_accounts.png"));
        item->setText(acc.apiurl);
        ui_.listServers->addItem(item);
    }
    ui_.listServers->blockSignals(false);


    on_listServers_currentRowChanged(-1);
}

SearchSettings::~SearchSettings()
{}

bool SearchSettings::validate() const 
{
    return true;
}

void SearchSettings::on_btnImport_clicked()
{
    std::vector<app::Newznab::Account> accounts;

    DlgImport dlg(this, accounts);
    dlg.exec();

    std::copy(std::begin(accounts), std::end(accounts),
        std::back_inserter(newznab_));

    ui_.listServers->blockSignals(true);
    for (const auto& acc : accounts)
    {
        auto* item = new QListWidgetItem();
        item->setIcon(QIcon("icons:ico_accounts.png"));
        item->setText(acc.apiurl);
        ui_.listServers->addItem(item);
    }
    ui_.listServers->blockSignals(false);
}

void SearchSettings::on_btnAdd_clicked()
{
    app::Newznab::Account acc;

    DlgNewznab dlg(this, acc);
    if (dlg.exec() == QDialog::Rejected)
        return;

    QListWidgetItem* item = new QListWidgetItem();
    item->setIcon(QIcon("icons:ico_accounts.png"));
    item->setText(acc.apiurl);
    ui_.listServers->addItem(item);

    newznab_.push_back(acc);
}

void SearchSettings::on_btnDel_clicked()
{
    const auto row = ui_.listServers->currentRow();
    Q_ASSERT(row != -1);

    const auto item = ui_.listServers->takeItem(row);
    delete item;

    auto it = std::begin(newznab_);
    std::advance(it, row);
    newznab_.erase(it);
    if (newznab_.empty())
    {
        ui_.btnEdit->setEnabled(false);
        ui_.btnDel->setEnabled(false);
    }
}

void SearchSettings::on_btnEdit_clicked()
{
    const auto row = ui_.listServers->currentRow();
    Q_ASSERT(row != -1);

    auto& acc = newznab_[row];
    DlgNewznab dlg(this, acc);
    if (dlg.exec() == QDialog::Rejected)
        return;

    QListWidgetItem* item = ui_.listServers->item(row);
    item->setText(acc.apiurl);
    ui_.listServers->editItem(item);
}

void SearchSettings::on_listServers_currentRowChanged(int currentRow)
{
    ui_.btnDel->setEnabled(currentRow != -1);
    ui_.btnEdit->setEnabled(currentRow != -1);
}

SearchModule::SearchModule()
{
    DEBUG("SearchModule UI created");
}

SearchModule::~SearchModule()
{
    DEBUG("SearchModule UI deleted");
}

void SearchModule::saveState(app::Settings& settings)
{
    QStringList list;
    for (std::size_t i=0; i<newznab_.size(); ++i)
    {
        const auto& acc = newznab_[i];
        const auto key  = QString("newznab-account-%1").arg(i);
        settings.set(key, "apiurl", acc.apiurl);
        settings.set(key, "apikey", acc.apikey);
        settings.set(key, "username", acc.username);
        settings.set(key, "password", acc.password);
        settings.set(key, "email", acc.email);
        list << key;
    }
    settings.set("newznab", "list", list);
}

void SearchModule::loadState(app::Settings& settings)
{
    const auto keys = settings.get("newznab", "list").toStringList();
    for (const auto& key : keys)
    {
        app::Newznab::Account acc;
        acc.apikey   = settings.get(key, "apikey").toString();
        acc.apiurl   = settings.get(key, "apiurl").toString();
        acc.username = settings.get(key, "username").toString();
        acc.password = settings.get(key, "password").toString();
        acc.email    = settings.get(key, "email").toString();
        newznab_.push_back(acc);        
    }
}

MainWidget* SearchModule::openSearch()
{
    Search* search = new Search(*this);
    QObject::connect(this, SIGNAL(listUpdated(const QStringList&)),
        search, SLOT(updateBackendList(const QStringList&)));    

    QStringList list;
    for (const auto& acc : newznab_)
        list << acc.apiurl;

    search->updateBackendList(list);
    return search;
}

SettingsWidget* SearchModule::getSettings()
{
    auto* p = new SearchSettings(newznab_);
    return p;
}

void SearchModule::applySettings(SettingsWidget* gui)
{
    auto* p = static_cast<SearchSettings*>(gui);
    newznab_ = std::move(p->newznab_);

    QStringList list;
    for (const auto& acc : newznab_)
    {
        list << acc.apiurl;
    }
    emit listUpdated(list);
}

void SearchModule::freeSettings(SettingsWidget* gui)
{
    delete gui;
}

const app::Newznab::Account& SearchModule::getAccount(const QString& apiurl) const 
{
    const auto it = std::find_if(std::begin(newznab_), std::end(newznab_),
        [&](const app::Newznab::Account& a) {
            return a.apiurl == apiurl;
        });
    ENDCHECK(newznab_, it);

    return *it;
}

} // gui