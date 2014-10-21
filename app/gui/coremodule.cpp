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

#define LOGTAG "core"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QFileDialog>
#  include <QDir>
#include <newsflash/warnpop.h>

#include "coremodule.h"
#include "../engine.h"
#include "../feedback.h"
#include "../settings.h"
#include "../accounts.h"
#include "../homedir.h"
#include "../debug.h"
#include "../format.h"


using app::str;

namespace gui
{

coresettings::coresettings()
{
    ui_.setupUi(this);
}

coresettings::~coresettings()
{}

bool coresettings::validate() const
{
    const auto& logs = ui_.editLogFiles->text();
    if (logs.isEmpty())
    {
        ui_.editLogFiles->setFocus();
        return false;
    }

    const auto& downloads = ui_.editDownloads->text();
    if (downloads.isEmpty())
    {
        ui_.editDownloads->setFocus();
        return false;
    }
    return true;
}

void coresettings::on_btnBrowseLog_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this, 
        tr("Select Log Folder"));
    if (dir.isEmpty())
        return;

    ui_.editLogFiles->setText(dir);
}

void coresettings::on_btnBrowseDownloads_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this,
        tr("Select Default Download Directory"));
    if (dir.isEmpty())
        return;

    ui_.editDownloads->setText(dir);
}

coremodule::coremodule()
{}

coremodule::~coremodule()
{}

void coremodule::loadstate(app::settings& s)
{
    // todo: load engine state
}

bool coremodule::savestate(app::settings& s)
{
    // todo: save engine state
    return true;
}

void coremodule::first_launch()
{}

gui::settings* coremodule::get_settings(app::settings& s)
{
    auto* ptr = new coresettings;
    auto& ui = ptr->ui_;

    const bool keepalive = s.get("settings", "keep_connections_alive", true);
    const bool overwrite = s.get("settings", "overwrite_existing_files", false);
    const bool discard   = s.get("settings", "discard_text_content", true);
    const bool updates   = s.get("settings", "check_for_software_updates", true);

    ui.chkKeepalive->setChecked(keepalive);
    ui.chkOverwriteExisting->setChecked(overwrite);
    ui.chkDiscardText->setChecked(discard);
    ui.chkUpdates->setChecked(updates);

    QString logs;
    QString files;

#if defined(LINUX_OS)
    // we use /tmp instead of /var/logs because most likely
    // we don't have permissions for /var/logs
    logs  = "/tmp/newsflash";
    files = QDir::toNativeSeparators(QDir::homePath() + "/Newsflash");
#elif defined(WINDOWS_OS)
    //logs = 
#endif

    logs  = s.get("paths", "logs", logs);
    files = s.get("paths", "files", files);

    ui.editLogFiles->setText(logs);
    ui.editDownloads->setText(files);

    const auto num_acc = app::g_accounts->num_accounts();
    for (std::size_t i=0; i<num_acc; ++i)
    {
        const auto& acc = app::g_accounts->get(i);
        ui.cmbMainAccount->addItem(acc.name);
        ui.cmbFillAccount->addItem(acc.name);
    }

    const auto* main = app::g_accounts->get_main_account();
    if (!main)
    {
        ui.rdbAskAccount->setChecked(true);
        ui.rdbUseAccount->setChecked(false);
    }
    else
    {
        ui.rdbAskAccount->setChecked(false);
        ui.rdbUseAccount->setChecked(true);
        ui.cmbMainAccount->setCurrentIndex(ui.cmbMainAccount->findText(main->name));
    }
    const auto* fill = app::g_accounts->get_fill_account();
    if (fill)
    {
        ui.grpFillAccount->setChecked(true);
        ui.cmbFillAccount->setCurrentIndex(ui.cmbFillAccount->findText(fill->name));
    }
    else
    {
        ui.grpFillAccount->setChecked(false);
    }

    return ptr;
}

void coremodule::apply_settings(settings* gui, app::settings& backend)
{
    auto* ptr = dynamic_cast<coresettings*>(gui);
    auto& ui = ptr->ui_;

    const bool keepalive = ui.chkKeepalive->isChecked();
    const bool overwrite = ui.chkOverwriteExisting->isChecked();
    const bool discard   = ui.chkDiscardText->isChecked();
    const bool updates   = ui.chkUpdates->isChecked();
    const auto logs  = ui.editLogFiles->text();
    const auto files = ui.editDownloads->text();

    backend.set("settings", "keep_connections_alive", keepalive);
    backend.set("settings", "overwrite_existing_files", overwrite);
    backend.set("settings", "discard_text_content", discard);
    backend.set("settings", "check_for_software_updates", updates);
    backend.set("paths", "logs", logs);
    backend.set("paths", "files", files);

    const auto ask_for_account = ui.rdbAskAccount->isChecked();
    const auto use_one_account = ui.rdbUseAccount->isChecked();
    if (ask_for_account)
    {
        app::g_accounts->set_main_account(0);
    }
    else
    {
        const auto name = ui.cmbMainAccount->currentText();
        const auto num_acc = app::g_accounts->num_accounts();
        for (std::size_t i=0; i<num_acc; ++i)
        {
            const auto& acc = app::g_accounts->get(i);
            if (acc.name != name)
                continue;

            app::g_accounts->set_main_account(acc.id);
            break;
        }
    }

    const auto enable_fill = ui.grpFillAccount->isChecked();
    if (!enable_fill)
    {
        app::g_accounts->set_fill_account(0);
    }
    else
    {
        const auto name = ui.cmbFillAccount->currentText();
        const auto num_acc = app::g_accounts->num_accounts();
        for (std::size_t i=0; i<num_acc; ++i)
        {
            const auto& acc = app::g_accounts->get(i);
            if (acc.name != name)
                continue;

            app::g_accounts->set_fill_account(acc.id);
            break;
        }
    }

    //app::g_engine->set_download_path(files);
    //app::g_engine->set_logfiles_path(logs);

    // todo: configure engine
    // todo: configure feedback 
    // todo: configure accounts
}

void coremodule::free_settings(settings* s)
{
    delete s;
}




} // gui