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

gui::settings* coremodule::get_settings(app::settings& s)
{
    auto* ptr = new coresettings;
    auto& ui = ptr->ui_;

    const auto overwrite = app::g_engine->get_overwrite_existing_files();
    const auto discard   = app::g_engine->get_discard_text_content();
    const auto logfiles  = app::g_engine->get_logfiles_path();
    const auto downloads = app::g_engine->get_download_path();
    const auto updates   = true; // todo:    

    ui.chkOverwriteExisting->setChecked(overwrite);
    ui.chkDiscardText->setChecked(discard);
    ui.chkUpdates->setChecked(updates);
    ui.editLogFiles->setText(logfiles);
    ui.editDownloads->setText(downloads);

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
    const auto logfiles  = ui.editLogFiles->text();
    const auto download  = ui.editDownloads->text();

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

    app::g_engine->set_downloads_path(download);
    app::g_engine->set_logfiles_path(logfiles);
    app::g_engine->set_overwrite_existing_files(overwrite);
    app::g_engine->set_discard_text_content(discard);

    // todo: throttle value.

    // todo: configure feedback 
}

void coremodule::free_settings(settings* s)
{
    delete s;
}

} // gui