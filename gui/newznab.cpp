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

NewznabSettings::NewznabSettings(std::vector<app::newznab::Account> accounts) : accounts_(std::move(accounts))
{
    ui_.setupUi(this);
    ui_.btnDel->setEnabled(false);
    ui_.btnEdit->setEnabled(false);

    ui_.listServers->blockSignals(true);
    for (const auto& acc : accounts_)
    {
        auto* item = new QListWidgetItem();
        item->setIcon(QIcon("icons:ico_accounts.png"));
        item->setText(acc.apiurl);
        ui_.listServers->addItem(item);
    }
    ui_.listServers->blockSignals(false);


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

    ui_.listServers->blockSignals(true);
    for (const auto& acc : accounts)
    {
        if (acc.apikey.isEmpty())
            continue;

        accounts_.push_back(acc);

        auto* item = new QListWidgetItem();
        item->setIcon(QIcon("icons:ico_accounts.png"));
        item->setText(acc.apiurl);
        ui_.listServers->addItem(item);
    }
    ui_.listServers->blockSignals(false);
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
    ui_.listServers->addItem(item);

    accounts_.push_back(acc);
}

void NewznabSettings::on_btnDel_clicked()
{
    const auto row = ui_.listServers->currentRow();
    Q_ASSERT(row != -1);

    const auto item = ui_.listServers->takeItem(row);
    delete item;

    auto it = std::begin(accounts_);
    std::advance(it, row);
    accounts_.erase(it);
    if (accounts_.empty())
    {
        ui_.btnEdit->setEnabled(false);
        ui_.btnDel->setEnabled(false);
    }
}

void NewznabSettings::on_btnEdit_clicked()
{
    const auto row = ui_.listServers->currentRow();
    Q_ASSERT(row != -1);

    auto& acc = accounts_[row];
    DlgNewznab dlg(this, acc);
    if (dlg.exec() == QDialog::Rejected)
        return;

    QListWidgetItem* item = ui_.listServers->item(row);
    item->setText(acc.apiurl);
    ui_.listServers->editItem(item);
}

void NewznabSettings::on_listServers_currentRowChanged(int currentRow)
{
    ui_.btnDel->setEnabled(currentRow != -1);
    ui_.btnEdit->setEnabled(currentRow != -1);
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
    for (std::size_t i=0; i<accounts_.size(); ++i)
    {
        const auto& acc = accounts_[i];
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
    settings.set("newznab", "enabled_streams", quint64(enabled_streams_.value()));
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
        accounts_.push_back(acc);
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

    enabled_streams_.set_from_value(streams);
}

MainWidget* Newznab::openSearch()
{
    Search* search = new Search(*this);
    QObject::connect(this, SIGNAL(listUpdated(const QStringList&)),
        search, SLOT(updateBackendList(const QStringList&)));

    QStringList list;
    for (const auto& acc : accounts_)
        list << acc.apiurl;

    search->updateBackendList(list);
    return search;
}

MainWidget* Newznab::openRSSFeed()
{
    RSS* rss = new RSS(*this);
    QObject::connect(this, SIGNAL(listUpdated(const QStringList&)),
        rss, SLOT(updateBackendList(const QStringList&)));

    QStringList list;
    for (const auto& account : accounts_)
        list << account.apiurl;

    rss->updateBackendList(list);
    return rss;
}

SettingsWidget* Newznab::getSettings()
{
    auto* p = new NewznabSettings(accounts_);
    auto& ui = p->ui_;

    using m = app::MediaType;
    if (enabled_streams_.test(m::TvInt)) ui.chkTvInt->setChecked(true);
    if (enabled_streams_.test(m::TvSD))  ui.chkTvSD->setChecked(true);
    if (enabled_streams_.test(m::TvHD))  ui.chkTvHD->setChecked(true);

    if (enabled_streams_.test(m::AppsPC)) ui.chkComputerPC->setChecked(true);
    if (enabled_streams_.test(m::AppsISO)) ui.chkComputerISO->setChecked(true);
    if (enabled_streams_.test(m::AppsAndroid)) ui.chkComputerAndroid->setChecked(true);
    if (enabled_streams_.test(m::AppsMac) || enabled_streams_.test(m::AppsIos))
        ui.chkComputerMac->setChecked(true);

    if (enabled_streams_.test(m::MusicMp3)) ui.chkMusicMP3->setChecked(true);
    if (enabled_streams_.test(m::MusicVideo)) ui.chkMusicVideo->setChecked(true);
    if (enabled_streams_.test(m::MusicLossless)) ui.chkMusicLosless->setChecked(true);

    if (enabled_streams_.test(m::MoviesInt)) ui.chkMoviesInt->setChecked(true);
    if (enabled_streams_.test(m::MoviesSD)) ui.chkMoviesSD->setChecked(true);
    if (enabled_streams_.test(m::MoviesHD)) ui.chkMoviesHD->setChecked(true);
    if (enabled_streams_.test(m::MoviesWMV)) ui.chkMoviesWMV->setChecked(true);

    if (enabled_streams_.test(m::GamesNDS) ||
        enabled_streams_.test(m::GamesWii))
        ui.chkConsoleNintendo->setChecked(true);

    if (enabled_streams_.test(m::GamesPSP) ||
        enabled_streams_.test(m::GamesPS2) ||
        enabled_streams_.test(m::GamesPS3) ||
        enabled_streams_.test(m::GamesPS4))
        ui.chkConsolePlaystation->setChecked(true);

    if (enabled_streams_.test(m::GamesXbox) ||
        enabled_streams_.test(m::GamesXbox360))
        ui.chkConsoleXbox->setChecked(true);

    if (enabled_streams_.test(m::AdultDVD)) ui.chkXXXDVD->setChecked(true);
    if (enabled_streams_.test(m::AdultSD)) ui.chkXXXSD->setChecked(true);
    if (enabled_streams_.test(m::AdultHD)) ui.chkXXXHD->setChecked(true);
    return p;
}

void Newznab::applySettings(SettingsWidget* gui)
{
    auto* p   = static_cast<NewznabSettings*>(gui);
    auto& ui  = p->ui_;
    accounts_ = std::move(p->accounts_);

    enabled_streams_.clear();

    using m = app::MediaType;

    if (ui.chkTvInt->isChecked()) enabled_streams_.set(m::TvInt);
    if (ui.chkTvSD->isChecked()) enabled_streams_.set(m::TvSD);
    if (ui.chkTvHD->isChecked()) enabled_streams_.set(m::TvHD);

    if (ui.chkComputerPC->isChecked()) enabled_streams_.set(m::AppsPC);
    if (ui.chkComputerISO->isChecked()) enabled_streams_.set(m::AppsISO);
    if (ui.chkComputerAndroid->isChecked()) enabled_streams_.set(m::AppsAndroid);
    if (ui.chkComputerMac->isChecked()) {
        enabled_streams_.set(m::AppsMac);
        enabled_streams_.set(m::AppsIos);
    }

    if (ui.chkMusicMP3->isChecked()) enabled_streams_.set(m::MusicMp3);
    if (ui.chkMusicVideo->isChecked()) enabled_streams_.set(m::MusicVideo);
    if (ui.chkMusicLosless->isChecked()) enabled_streams_.set(m::MusicLossless);

    if (ui.chkMoviesInt->isChecked()) enabled_streams_.set(m::MoviesInt);
    if (ui.chkMoviesSD->isChecked()) enabled_streams_.set(m::MoviesSD);
    if (ui.chkMoviesHD->isChecked()) enabled_streams_.set(m::MoviesHD);
    if (ui.chkMoviesWMV->isChecked()) enabled_streams_.set(m::MoviesWMV);

    if (ui.chkConsoleNintendo->isChecked())
    {
        enabled_streams_.set(m::GamesNDS);
        enabled_streams_.set(m::GamesWii);
    }
    if (ui.chkConsolePlaystation->isChecked())
    {
        enabled_streams_.set(m::GamesPSP);
        enabled_streams_.set(m::GamesPS2);
        enabled_streams_.set(m::GamesPS3);
        enabled_streams_.set(m::GamesPS4);
    }
    if (ui.chkConsoleXbox->isChecked())
    {
        enabled_streams_.set(m::GamesXbox);
        enabled_streams_.set(m::GamesXbox360);
    }

    if (ui.chkXXXDVD->isChecked()) enabled_streams_.set(m::AdultDVD);
    if (ui.chkXXXHD->isChecked()) enabled_streams_.set(m::AdultHD);
    if (ui.chkXXXSD->isChecked()) enabled_streams_.set(m::AdultSD);

    // notify about change in the list of accounts.
    QStringList list;
    for (const auto& acc : accounts_)
    {
        list << acc.apiurl;
    }
    emit listUpdated(list);
}

void Newznab::freeSettings(SettingsWidget* gui)
{
    delete gui;
}

std::unique_ptr<app::Indexer> Newznab::makeSearchEngine(const QString& hostName)
{
    const auto& account = findAccount(hostName);

    return std::make_unique<app::newznab::Search>(account, enabled_streams_);
}

std::unique_ptr<app::RSSFeed> Newznab::makeRSSFeedEngine(const QString& hostName)
{
    const auto& account = findAccount(hostName);

    return std::make_unique<app::newznab::RSSFeed>(account, enabled_streams_);
}


const app::newznab::Account& Newznab::findAccount(const QString& apiurl) const
{
    const auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [&](const app::newznab::Account& a) {
            return a.apiurl == apiurl;
        });
    ENDCHECK(accounts_, it);

    return *it;
}

} // gui
