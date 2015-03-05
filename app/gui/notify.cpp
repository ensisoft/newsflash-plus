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

#define LOGTAG "notify"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QIcon>
#  include "ui_notify.h"
#include <newsflash/warnpop.h>
#include "settings.h"
#include "notify.h"
#include "../debug.h"
#include "../eventlog.h"
#include "../settings.h"
#include "../fileinfo.h"
#include "../archive.h"
#include "../event.h"

namespace {

    // we can tuck this away insice the .cpp because 
    // this settings widget doesn't require any signals/slots
class MySettings : public gui::SettingsWidget
{
public:
    MySettings()
    {
        ui_.setupUi(this);
    }
private:
    friend class gui::Notify;
    Ui::Notify ui_;
};

} // namespace

namespace gui
{

Notify::Notify()
{
    restore_  = new QAction(tr("Restore from tray"), this);
    restore_->setIcon(QIcon("icons:ico_app_restore.png"));
    restore_->setEnabled(false);
    QObject::connect(restore_, SIGNAL(triggered()), this, SIGNAL(restore()));
    QObject::connect(restore_, SIGNAL(triggered()), this, SLOT(actionRestore()));

    minimize_ = new QAction(tr("Minimize to tray"), this);
    minimize_->setIcon(QIcon("icons:ico_app_minimize.png"));
    QObject::connect(minimize_, SIGNAL(triggered()), this, SIGNAL(minimize()));
    QObject::connect(minimize_, SIGNAL(triggered()), this, SLOT(actionMinimize()));

    exit_ = new QAction(tr("Exit application"), this);
    exit_->setIcon(QIcon("icons:ico_app_exit.png"));
    QObject::connect(exit_, SIGNAL(triggered()), this, SIGNAL(exit()));

    when_.set(NotifyWhen::Enable);
    when_.set(NotifyWhen::OnFile);
    when_.set(NotifyWhen::OnFilePack);
    when_.set(NotifyWhen::OnError);
    when_.set(NotifyWhen::OnWarning);
    when_.set(NotifyWhen::OnRepair);
    when_.set(NotifyWhen::OnUnpack);

    tray_.setIcon(QIcon("icons:ico_newsflash.png"));
    tray_.setToolTip(NEWSFLASH_TITLE " " NEWSFLASH_VERSION);
    tray_.setVisible(true);

    QObject::connect(&tray_, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
        this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));

    DEBUG("Notify UI created");
}

Notify::~Notify()
{
    DEBUG("Notify UI deleted.");
}

bool Notify::addActions(QMenu& menu)
{
    menu.addAction(minimize_);
    return true;
}

void Notify::loadState(app::Settings& settings)
{
    const auto bits = when_.value();

    when_.set_from_value(settings.get("notify", "bits", bits));
}

void Notify::saveState(app::Settings& settings)
{
    const auto bits = when_.value();

    settings.set("notify", "bits", bits);
}

SettingsWidget* Notify::getSettings() 
{
    auto* p = new MySettings;
    auto& ui = p->ui_;
    ui.chkNotifyFile->setChecked(when_.test(NotifyWhen::OnFile));
    ui.chkNotifyBatch->setChecked(when_.test(NotifyWhen::OnFilePack));
    ui.chkNotifyError->setChecked(when_.test(NotifyWhen::OnError));
    ui.chkNotifyWarning->setChecked(when_.test(NotifyWhen::OnWarning));
    ui.chkNotifyRepair->setChecked(when_.test(NotifyWhen::OnRepair));
    ui.chkNotifyExtract->setChecked(when_.test(NotifyWhen::OnUnpack));
    ui.grpEnable->setChecked(when_.test(NotifyWhen::Enable));
    return p;
}

void Notify::applySettings(SettingsWidget* widget)
{
    auto* p  = dynamic_cast<MySettings*>(widget);
    auto& ui = p->ui_;

    when_.set(NotifyWhen::Enable, ui.grpEnable->isChecked());
    when_.set(NotifyWhen::OnFile, ui.chkNotifyFile->isChecked());
    when_.set(NotifyWhen::OnFilePack, ui.chkNotifyBatch->isChecked());
    when_.set(NotifyWhen::OnError, ui.chkNotifyError->isChecked());
    when_.set(NotifyWhen::OnWarning, ui.chkNotifyWarning->isChecked());
    when_.set(NotifyWhen::OnRepair, ui.chkNotifyRepair->isChecked());
    when_.set(NotifyWhen::OnUnpack, ui.chkNotifyExtract->isChecked());
}

void Notify::fileCompleted(const app::FileInfo& file)
{
    if (!when_.test(NotifyWhen::Enable))
        return;

    if (!when_.test(NotifyWhen::OnFile))
        return;
    tray_.showMessage(tr("File complete"), file.name);
}

void Notify::packCompleted(const app::FilePackInfo& pack)
{
    if (!when_.test(NotifyWhen::Enable))
        return;

    if (!when_.test(NotifyWhen::OnFilePack))
        return;
    tray_.showMessage(tr("File batch complete"), pack.desc);
}

void Notify::newEvent(const app::Event& event)
{
    if (!when_.test(NotifyWhen::Enable))
        return;

    if (event.type == app::Event::Type::Error)
    {
        if (!when_.test(NotifyWhen::OnError))
            return;
        tray_.showMessage(tr("Error occurred"), event.message);
    }
    else if (event.type == app::Event::Type::Warning)
    {
        if (!when_.test(NotifyWhen::OnWarning))
            return;
        tray_.showMessage(tr("Warning occurred"), event.message);            
    }
}

void Notify::repairReady(const app::Archive& arc)
{
    if (!when_.test(NotifyWhen::Enable))
        return;

    if (!when_.test(NotifyWhen::OnRepair))
        return;

    if (arc.state == app::Archive::Status::Error ||
        arc.state == app::Archive::Status::Failed)
    {
        tray_.showMessage(tr("Repair failed"), 
            tr("%1\n%2").arg(arc.desc).arg(arc.message));
    }
    else
    {
        tray_.showMessage(tr("Repair ready"),
            tr("%1\n%2").arg(arc.desc).arg(arc.message));        
    }

}

void Notify::unpackReady(const app::Archive& arc)
{
    if (!when_.test(NotifyWhen::Enable))
        return;

    if (!when_.test(NotifyWhen::OnUnpack))
        return;

    if (arc.state == app::Archive::Status::Error ||
        arc.state == app::Archive::Status::Failed)
    {
        tray_.showMessage(tr("Unpacking failed"),
            tr("%1\n%2").arg(arc.desc).arg(arc.message));
    }
    else
    {
        tray_.showMessage(tr("Unpacking ready"),
            tr("%1\n%2").arg(arc.desc).arg(arc.message));        
    }
}

void Notify::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Context)
        return;

    QMenu menu;
    menu.addAction(restore_);
    menu.addSeparator();
    menu.addAction(exit_);
    menu.exec(QCursor::pos());
}

void Notify::actionRestore()
{
    minimize_->setEnabled(true);
}

void Notify::actionMinimize()
{
    restore_->setEnabled(true);
}

} // gui
