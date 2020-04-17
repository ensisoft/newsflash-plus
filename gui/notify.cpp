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

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QMenu>
#  include <QIcon>
#  include "ui_notify.h"
#include "newsflash/warnpop.h"
#include "settings.h"
#include "notify.h"
#include "app/debug.h"
#include "app/eventlog.h"
#include "app/settings.h"
#include "app/fileinfo.h"
#include "app/archive.h"
#include "app/event.h"

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
    m_restoreAction  = new QAction(tr("Restore from Tray"), this);
    m_restoreAction->setIcon(QIcon("icons:ico_app_restore.png"));
    m_restoreAction->setEnabled(false);
    QObject::connect(m_restoreAction, SIGNAL(triggered()), this, SIGNAL(restore()));
    QObject::connect(m_restoreAction, SIGNAL(triggered()), this, SLOT(actionRestore()));

    m_minimizeAction = new QAction(tr("Minimize to Tray"), this);
    m_minimizeAction->setIcon(QIcon("icons:ico_app_minimize.png"));
    QObject::connect(m_minimizeAction, SIGNAL(triggered()), this, SIGNAL(minimize()));
    QObject::connect(m_minimizeAction, SIGNAL(triggered()), this, SLOT(actionMinimize()));

    m_exitAction = new QAction(tr("Exit Application"), this);
    m_exitAction->setIcon(QIcon("icons:ico_app_exit.png"));
    QObject::connect(m_exitAction, SIGNAL(triggered()), this, SIGNAL(exit()));

    m_notifications.set(NotifyWhen::Enable);
    m_notifications.set(NotifyWhen::OnFile);
    m_notifications.set(NotifyWhen::OnFilePack);
    m_notifications.set(NotifyWhen::OnError);
    m_notifications.set(NotifyWhen::OnWarning);
    m_notifications.set(NotifyWhen::OnRepair);
    m_notifications.set(NotifyWhen::OnUnpack);

    m_trayIcon.setIcon(QIcon("icons:ico_newsflash.png"));
    m_trayIcon.setToolTip(NEWSFLASH_TITLE " " NEWSFLASH_VERSION);
    m_trayIcon.setVisible(true);

    QObject::connect(&m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
        this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));

    DEBUG("Notify UI created");
}

Notify::~Notify()
{
    DEBUG("Notify UI deleted.");
}

bool Notify::addActions(QMenu& menu)
{
    menu.addAction(m_minimizeAction);
    return true;
}

void Notify::loadState(app::Settings& settings)
{
    const auto bits = m_notifications.value();

    m_notifications.set_from_value(settings.get("notify", "bits", bits));
}

void Notify::saveState(app::Settings& settings)
{
    const auto bits = m_notifications.value();

    settings.set("notify", "bits", bits);
}

SettingsWidget* Notify::getSettings()
{
    auto* p = new MySettings;
    auto& ui = p->ui_;
    ui.chkNotifyFile->setChecked(m_notifications.test(NotifyWhen::OnFile));
    ui.chkNotifyBatch->setChecked(m_notifications.test(NotifyWhen::OnFilePack));
    ui.chkNotifyError->setChecked(m_notifications.test(NotifyWhen::OnError));
    ui.chkNotifyWarning->setChecked(m_notifications.test(NotifyWhen::OnWarning));
    ui.chkNotifyRepair->setChecked(m_notifications.test(NotifyWhen::OnRepair));
    ui.chkNotifyExtract->setChecked(m_notifications.test(NotifyWhen::OnUnpack));
    ui.grpEnable->setChecked(m_notifications.test(NotifyWhen::Enable));
    return p;
}

void Notify::applySettings(SettingsWidget* widget)
{
    auto* p  = dynamic_cast<MySettings*>(widget);
    auto& ui = p->ui_;

    m_notifications.set(NotifyWhen::Enable, ui.grpEnable->isChecked());
    m_notifications.set(NotifyWhen::OnFile, ui.chkNotifyFile->isChecked());
    m_notifications.set(NotifyWhen::OnFilePack, ui.chkNotifyBatch->isChecked());
    m_notifications.set(NotifyWhen::OnError, ui.chkNotifyError->isChecked());
    m_notifications.set(NotifyWhen::OnWarning, ui.chkNotifyWarning->isChecked());
    m_notifications.set(NotifyWhen::OnRepair, ui.chkNotifyRepair->isChecked());
    m_notifications.set(NotifyWhen::OnUnpack, ui.chkNotifyExtract->isChecked());
}

void Notify::fileCompleted(const app::FileInfo& file)
{
    if (!m_notifications.test(NotifyWhen::Enable))
        return;

    if (!m_notifications.test(NotifyWhen::OnFile))
        return;
    m_trayIcon.showMessage(tr("File complete"), file.name);
}

void Notify::packCompleted(const app::FilePackInfo& pack)
{
    if (!m_notifications.test(NotifyWhen::Enable))
        return;

    if (!m_notifications.test(NotifyWhen::OnFilePack))
        return;
    m_trayIcon.showMessage(tr("File batch complete"), pack.desc);
}

void Notify::newEvent(const app::Event& event)
{
    if (!m_notifications.test(NotifyWhen::Enable))
        return;

    if (event.type == app::Event::Type::Error)
    {
        if (!m_notifications.test(NotifyWhen::OnError))
            return;
        m_trayIcon.showMessage(tr("Error"), event.message);
    }
    else if (event.type == app::Event::Type::Warning)
    {
        if (!m_notifications.test(NotifyWhen::OnWarning))
            return;
        m_trayIcon.showMessage(tr("Warning"), event.message);
    }
}

void Notify::repairReady(const app::Archive& arc)
{
    if (!m_notifications.test(NotifyWhen::Enable))
        return;

    if (!m_notifications.test(NotifyWhen::OnRepair))
        return;

    if (arc.state == app::Archive::Status::Error ||
        arc.state == app::Archive::Status::Failed)
    {
        m_trayIcon.showMessage(tr("Repair failed"),
            tr("%1\n%2").arg(arc.desc).arg(arc.message));
    }
    else
    {
        m_trayIcon.showMessage(tr("Repair ready"),
            tr("%1\n%2").arg(arc.desc).arg(arc.message));
    }

}

void Notify::unpackReady(const app::Archive& arc)
{
    if (!m_notifications.test(NotifyWhen::Enable))
        return;

    if (!m_notifications.test(NotifyWhen::OnUnpack))
        return;

    if (arc.state == app::Archive::Status::Error ||
        arc.state == app::Archive::Status::Failed)
    {
        m_trayIcon.showMessage(tr("Unpacking failed"),
            tr("%1\n%2").arg(arc.desc).arg(arc.message));
    }
    else
    {
        m_trayIcon.showMessage(tr("Unpacking ready"),
            tr("%1\n%2").arg(arc.desc).arg(arc.message));
    }
}

void Notify::windowRestored()
{
    m_minimizeAction->setEnabled(true);
    m_restoreAction->setEnabled(false);
}

void Notify::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Context)
        return;

    QMenu menu;
    menu.addAction(m_restoreAction);
    menu.addSeparator();
    menu.addAction(m_exitAction);
    menu.exec(QCursor::pos());
}

void Notify::actionRestore()
{
    m_minimizeAction->setEnabled(true);
}

void Notify::actionMinimize()
{
    m_restoreAction->setEnabled(true);
}

} // gui
