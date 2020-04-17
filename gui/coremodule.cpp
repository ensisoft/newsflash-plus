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

#define LOGTAG "core"

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QFileDialog>
#  include <QMessageBox>
#  include <QDir>
#include "newsflash/warnpop.h"

#include <cmath>

#include "coremodule.h"
#include "mainwindow.h"
#include "app/engine.h"
#include "app/feedback.h"
#include "app/settings.h"
#include "app/accounts.h"
#include "app/homedir.h"
#include "app/debug.h"
#include "app/format.h"
#include "app/telephone.h"
#include "app/utility.h"
#include "app/format.h"
#include "app/types.h"

namespace gui
{

CoreSettings::CoreSettings()
{
    ui_.setupUi(this);
}

CoreSettings::~CoreSettings()
{}

bool CoreSettings::validate() const
{
    return true;
}

void CoreSettings::on_sliderThrottle_valueChanged(int val)
{
    //const auto bytes = val * app::KB(5);

    const auto x = (double)val / (double)ui_.sliderThrottle->maximum();
    const auto y = std::pow(x, 2.5);
    const auto bytes = app::MB(100) * y;

    ui_.editThrottle->setText(app::toString(app::speed{(quint32)bytes}));
}


CoreModule::CoreModule()
{}

CoreModule::~CoreModule()
{}

void CoreModule::loadState(app::Settings& s)
{
    check_for_updates_ = s.get("settings", "check_for_software_updates", true);
    if (check_for_updates_)
    {
        et_.reset(new app::Telephone);
        QObject::connect(et_.get(), SIGNAL(completed(bool, QString)),
            this, SLOT(calledHome(bool, QString)));
        et_->callhome();
    }
}

void CoreModule::saveState(app::Settings& s)
{
    s.set("settings", "check_for_software_updates", check_for_updates_);
}

SettingsWidget* CoreModule::getSettings()
{
    auto* ptr = new CoreSettings;
    auto& ui = ptr->ui_;

    const auto updates = check_for_updates_; //s.get("settings", "check_for_software_updates", true);

    ui.chkUpdates->setChecked(updates);

    const auto num_acc = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<num_acc; ++i)
    {
        const auto& acc = app::g_accounts->getAccount(i);
        ui.cmbMainAccount->addItem(acc.name);
        ui.cmbFillAccount->addItem(acc.name);
    }

    const auto* main = app::g_accounts->getMainAccount();
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
    const auto* fill = app::g_accounts->getFillAccount();
    if (fill)
    {
        ui.grpFillAccount->setChecked(true);
        ui.cmbFillAccount->setCurrentIndex(ui.cmbFillAccount->findText(fill->name));
    }
    else
    {
        ui.grpFillAccount->setChecked(false);
    }


    auto throttle = app::g_engine->getThrottleValue();
    if (!throttle)
        throttle = app::KB(5);

    const auto y = (double)throttle / (double)app::MB(100);
    const auto x = std::pow(y, 1.0/2.5);
    const auto t = 100 * x;

    ui.sliderThrottle->setMaximum(99);
    ui.sliderThrottle->setMinimum(1);
    ui.sliderThrottle->setValue(t);

    ui.editThrottle->setText(app::toString(app::speed{(quint32)(y * app::MB(100))}));

    return ptr;
}

void CoreModule::applySettings(SettingsWidget* gui)
{
    auto* ptr = dynamic_cast<CoreSettings*>(gui);
    auto& ui = ptr->ui_;

    const bool updates = ui.chkUpdates->isChecked();

    const auto ask_for_account = ui.rdbAskAccount->isChecked();
    //const auto use_one_account = ui.rdbUseAccount->isChecked();
    if (ask_for_account)
    {
        app::g_accounts->setMainAccount(0);
    }
    else
    {
        const auto name = ui.cmbMainAccount->currentText();
        const auto num_acc = app::g_accounts->numAccounts();
        for (std::size_t i=0; i<num_acc; ++i)
        {
            const auto& acc = app::g_accounts->getAccount(i);
            if (acc.name != name)
                continue;

            app::g_accounts->setMainAccount(acc.id);
            break;
        }
    }

    const auto enable_fill = ui.grpFillAccount->isChecked();
    if (!enable_fill)
    {
        app::g_accounts->setFillAccount(0);
    }
    else
    {
        const auto name = ui.cmbFillAccount->currentText();
        const auto num_acc = app::g_accounts->numAccounts();
        for (std::size_t i=0; i<num_acc; ++i)
        {
            const auto& acc = app::g_accounts->getAccount(i);
            if (acc.name != name)
                continue;

            app::g_accounts->setFillAccount(acc.id);
            break;
        }
    }



    const auto tick  = ui.sliderThrottle->value();
    const auto ticks = ui.sliderThrottle->maximum();
    const auto x = (double)tick / (double)ticks;
    const auto y = std::pow(x, 2.5);
    const auto throttle = y * app::MB(100);

    app::g_engine->setThrottleValue(throttle);

    DEBUG("Throttle value %1", app::speed{(quint32)throttle});

    check_for_updates_ = updates;
}

void CoreModule::freeSettings(SettingsWidget* s)
{
    delete s;
}

void CoreModule::calledHome(bool new_version_available, QString latest)
{
    if (new_version_available)
    {
        QMessageBox::information(g_win,
            tr("A New Version Is Available"),
            tr("A new version of the software is available.\n"
               "Current version %1\n"
               "Latest version %2\n\n"
               "The latest software can be downloaded from www.ensisoft.com").arg(NEWSFLASH_VERSION)
            .arg(latest),
            QMessageBox::Ok);
    }
    et_.release();
}

} // gui
