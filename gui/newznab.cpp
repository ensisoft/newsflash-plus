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

#define LOGTAG "newznab"

#include "newsflash/config.h"

#include "newznab.h"
#include "search.h"
#include "rss.h"
#include "dlgnewznab.h"
#include "dlgimport.h"
#include "app/settings.h"
#include "app/debug.h"
#include "app/newznab.h"

namespace gui
{

NewznabSettings::NewznabSettings(std::vector<app::newznab::Account> accounts) : mAccounts(std::move(accounts))
{
    mUi.setupUi(this);
    mUi.btnDel->setEnabled(false);
    mUi.btnEdit->setEnabled(false);

    mUi.listServers->blockSignals(true);
    for (const auto& acc : mAccounts)
    {
        auto* item = new QListWidgetItem();
        item->setIcon(QIcon("icons:ico_accounts.png"));
        item->setText(acc.apiurl);
        mUi.listServers->addItem(item);
    }
    mUi.listServers->blockSignals(false);


    on_listServers_currentRowChanged(-1);
}

NewznabSettings::~NewznabSettings()
{}

bool NewznabSettings::validate() const
{
    return true;
}

void NewznabSettings::on_btnImport_clicked()
{
    std::vector<app::newznab::Account> accounts;

    DlgImport dlg(this, accounts);
    dlg.exec();

    mUi.listServers->blockSignals(true);
    for (const auto& acc : accounts)
    {
        mAccounts.push_back(acc);

        auto* item = new QListWidgetItem();
        item->setIcon(QIcon("icons:ico_accounts.png"));
        item->setText(acc.apiurl);
        mUi.listServers->addItem(item);
    }
    mUi.listServers->blockSignals(false);
}

void NewznabSettings::on_btnAdd_clicked()
{
    app::newznab::Account acc;

    DlgNewznab dlg(this, acc);
    if (dlg.exec() == QDialog::Rejected)
        return;

    QListWidgetItem* item = new QListWidgetItem();
    item->setIcon(QIcon("icons:ico_accounts.png"));
    item->setText(acc.apiurl);
    mUi.listServers->addItem(item);

    mAccounts.push_back(acc);
}

void NewznabSettings::on_btnDel_clicked()
{
    const auto row = mUi.listServers->currentRow();
    Q_ASSERT(row != -1);

    const auto item = mUi.listServers->takeItem(row);
    delete item;

    auto it = std::begin(mAccounts);
    std::advance(it, row);
    mAccounts.erase(it);
    if (mAccounts.empty())
    {
        mUi.btnEdit->setEnabled(false);
        mUi.btnDel->setEnabled(false);
    }
}

void NewznabSettings::on_btnEdit_clicked()
{
    const auto row = mUi.listServers->currentRow();
    Q_ASSERT(row != -1);

    auto& acc = mAccounts[row];
    DlgNewznab dlg(this, acc);
    if (dlg.exec() == QDialog::Rejected)
        return;

    QListWidgetItem* item = mUi.listServers->item(row);
    item->setText(acc.apiurl);
    mUi.listServers->editItem(item);
}

void NewznabSettings::on_listServers_currentRowChanged(int currentRow)
{
    mUi.btnDel->setEnabled(currentRow != -1);
    mUi.btnEdit->setEnabled(currentRow != -1);
}

Newznab::Newznab()
{
    DEBUG("Newznab MainModule created");
}

Newznab::~Newznab()
{
    DEBUG("Newznab MainModule deleted");
}

void Newznab::saveState(app::Settings& settings)
{
    QStringList list;
    for (std::size_t i=0; i<mAccounts.size(); ++i)
    {
        const auto& acc = mAccounts[i];
        const auto key  = QString("newznab-account-%1").arg(i);
        settings.set(key, "apiurl", acc.apiurl);
        settings.set(key, "apikey", acc.apikey);
        settings.set(key, "username", acc.username);
        settings.set(key, "password", acc.password);
        settings.set(key, "email", acc.email);
        settings.set(key, "userid", acc.userid);
        list << key;
    }
    settings.set("newznab", "list", list);
    settings.set("newznab", "enabled_streams", quint64(mEnabledStreams.value()));
}

void Newznab::loadState(app::Settings& settings)
{
    const auto keys = settings.get("newznab", "list").toStringList();
    for (const auto& key : keys)
    {
        app::newznab::Account acc;
        acc.apikey   = settings.get(key, "apikey").toString();
        acc.apiurl   = settings.get(key, "apiurl").toString();
        acc.username = settings.get(key, "username").toString();
        acc.password = settings.get(key, "password").toString();
        acc.email    = settings.get(key, "email").toString();
        acc.userid   = settings.get(key, "userid").toString();
        mAccounts.push_back(acc);
    }

    using m = app::MediaType;

    app::MediaTypeFlag defaults;
    defaults.set(m::TvInt);
    defaults.set(m::TvSD);
    defaults.set(m::TvHD);

    defaults.set(m::AppsPC);
    defaults.set(m::AppsISO);
    defaults.set(m::AppsAndroid);
    defaults.set(m::AppsMac);
    defaults.set(m::AppsIos);

    defaults.set(m::MusicMp3);
    defaults.set(m::MusicVideo);
    defaults.set(m::MusicLossless);

    defaults.set(m::MoviesInt);
    defaults.set(m::MoviesSD);
    defaults.set(m::MoviesHD);
    defaults.set(m::MoviesWMV);

    defaults.set(m::GamesNDS);
    defaults.set(m::GamesWii);
    defaults.set(m::GamesPS1);
    defaults.set(m::GamesPS2);
    defaults.set(m::GamesPS3);
    defaults.set(m::GamesPS4);
    defaults.set(m::GamesXbox);
    defaults.set(m::GamesXbox360);

    defaults.set(m::AdultDVD);
    defaults.set(m::AdultSD);
    defaults.set(m::AdultHD);

    quint64 streams = defaults.value();

    streams = settings.get("newznab", "enabled_streams", streams);

    mEnabledStreams.set_from_value(streams);
}

SettingsWidget* Newznab::getSettings()
{
    auto* p = new NewznabSettings(mAccounts);
    auto& ui = p->mUi;

    using m = app::MediaType;
    if (mEnabledStreams.test(m::TvInt)) ui.chkTvInt->setChecked(true);
    if (mEnabledStreams.test(m::TvSD))  ui.chkTvSD->setChecked(true);
    if (mEnabledStreams.test(m::TvHD))  ui.chkTvHD->setChecked(true);

    if (mEnabledStreams.test(m::AppsPC)) ui.chkComputerPC->setChecked(true);
    if (mEnabledStreams.test(m::AppsISO)) ui.chkComputerISO->setChecked(true);
    if (mEnabledStreams.test(m::AppsAndroid)) ui.chkComputerAndroid->setChecked(true);
    if (mEnabledStreams.test(m::AppsMac) || mEnabledStreams.test(m::AppsIos))
        ui.chkComputerMac->setChecked(true);

    if (mEnabledStreams.test(m::MusicMp3)) ui.chkMusicMP3->setChecked(true);
    if (mEnabledStreams.test(m::MusicVideo)) ui.chkMusicVideo->setChecked(true);
    if (mEnabledStreams.test(m::MusicLossless)) ui.chkMusicLosless->setChecked(true);

    if (mEnabledStreams.test(m::MoviesInt)) ui.chkMoviesInt->setChecked(true);
    if (mEnabledStreams.test(m::MoviesSD)) ui.chkMoviesSD->setChecked(true);
    if (mEnabledStreams.test(m::MoviesHD)) ui.chkMoviesHD->setChecked(true);
    if (mEnabledStreams.test(m::MoviesWMV)) ui.chkMoviesWMV->setChecked(true);

    if (mEnabledStreams.test(m::GamesNDS) ||
        mEnabledStreams.test(m::GamesWii))
        ui.chkConsoleNintendo->setChecked(true);

    if (mEnabledStreams.test(m::GamesPSP) ||
        mEnabledStreams.test(m::GamesPS2) ||
        mEnabledStreams.test(m::GamesPS3) ||
        mEnabledStreams.test(m::GamesPS4))
        ui.chkConsolePlaystation->setChecked(true);

    if (mEnabledStreams.test(m::GamesXbox) ||
        mEnabledStreams.test(m::GamesXbox360))
        ui.chkConsoleXbox->setChecked(true);

    if (mEnabledStreams.test(m::AdultDVD)) ui.chkXXXDVD->setChecked(true);
    if (mEnabledStreams.test(m::AdultSD)) ui.chkXXXSD->setChecked(true);
    if (mEnabledStreams.test(m::AdultHD)) ui.chkXXXHD->setChecked(true);
    return p;
}

void Newznab::applySettings(SettingsWidget* gui)
{
    auto* p   = static_cast<NewznabSettings*>(gui);
    auto& ui  = p->mUi;
    mAccounts = std::move(p->mAccounts);

    mEnabledStreams.clear();

    using m = app::MediaType;

    if (ui.chkTvInt->isChecked()) mEnabledStreams.set(m::TvInt);
    if (ui.chkTvSD->isChecked()) mEnabledStreams.set(m::TvSD);
    if (ui.chkTvHD->isChecked()) mEnabledStreams.set(m::TvHD);

    if (ui.chkComputerPC->isChecked()) mEnabledStreams.set(m::AppsPC);
    if (ui.chkComputerISO->isChecked()) mEnabledStreams.set(m::AppsISO);
    if (ui.chkComputerAndroid->isChecked()) mEnabledStreams.set(m::AppsAndroid);
    if (ui.chkComputerMac->isChecked()) {
        mEnabledStreams.set(m::AppsMac);
        mEnabledStreams.set(m::AppsIos);
    }

    if (ui.chkMusicMP3->isChecked()) mEnabledStreams.set(m::MusicMp3);
    if (ui.chkMusicVideo->isChecked()) mEnabledStreams.set(m::MusicVideo);
    if (ui.chkMusicLosless->isChecked()) mEnabledStreams.set(m::MusicLossless);

    if (ui.chkMoviesInt->isChecked()) mEnabledStreams.set(m::MoviesInt);
    if (ui.chkMoviesSD->isChecked()) mEnabledStreams.set(m::MoviesSD);
    if (ui.chkMoviesHD->isChecked()) mEnabledStreams.set(m::MoviesHD);
    if (ui.chkMoviesWMV->isChecked()) mEnabledStreams.set(m::MoviesWMV);

    if (ui.chkConsoleNintendo->isChecked())
    {
        mEnabledStreams.set(m::GamesNDS);
        mEnabledStreams.set(m::GamesWii);
    }
    if (ui.chkConsolePlaystation->isChecked())
    {
        mEnabledStreams.set(m::GamesPSP);
        mEnabledStreams.set(m::GamesPS2);
        mEnabledStreams.set(m::GamesPS3);
        mEnabledStreams.set(m::GamesPS4);
    }
    if (ui.chkConsoleXbox->isChecked())
    {
        mEnabledStreams.set(m::GamesXbox);
        mEnabledStreams.set(m::GamesXbox360);
    }

    if (ui.chkXXXDVD->isChecked()) mEnabledStreams.set(m::AdultDVD);
    if (ui.chkXXXHD->isChecked()) mEnabledStreams.set(m::AdultHD);
    if (ui.chkXXXSD->isChecked()) mEnabledStreams.set(m::AdultSD);

    // notify about change in the list of accounts.
    QStringList list;
    for (const auto& acc : mAccounts)
    {
        list << acc.apiurl;
    }
    emit listUpdated(list);
}

void Newznab::freeSettings(SettingsWidget* gui)
{
    delete gui;
}

QStringList Newznab::listAccounts() const
{
    QStringList list;
    for (const auto& acc : mAccounts) {
        list << acc.apiurl;
    }
    return list;
}

std::unique_ptr<app::Indexer> Newznab::makeSearchEngine(const QString& hostName)
{
    const auto& account = findAccount(hostName);

    return std::make_unique<app::newznab::Search>(account, mEnabledStreams);
}

std::unique_ptr<app::RSSFeed> Newznab::makeRSSFeedEngine(const QString& hostName)
{
    const auto& account = findAccount(hostName);

    return std::make_unique<app::newznab::RSSFeed>(account, mEnabledStreams);
}


const app::newznab::Account& Newznab::findAccount(const QString& apiurl) const
{
    const auto it = std::find_if(std::begin(mAccounts), std::end(mAccounts),
        [&](const app::newznab::Account& a) {
            return a.apiurl == apiurl;
        });
    ENDCHECK(mAccounts, it);

    return *it;
}

} // gui
