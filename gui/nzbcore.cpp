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

#define LOGTAG "nzb"

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QtGui/QFileDialog>
#  include <QtGui/QAction>
#  include <QtGui/QMenu>
#  include <QDir>
#include "newsflash/warnpop.h"
#include "dlgdragdrop.h"
#include "mainwindow.h"
#include "nzbcore.h"
#include "nzbfile.h"
#include "common.h"
#include "app/settings.h"
#include "app/debug.h"

namespace gui
{

NZBSettings::NZBSettings()
{
    ui_.setupUi(this);
}

NZBSettings::~NZBSettings()
{}

bool NZBSettings::validate() const
{
    return true;
}

void NZBSettings::on_btnAddWatchFolder_clicked()
{
    ui_.btnDelWatchFolder->setEnabled(true);

    auto dir = QFileDialog::getExistingDirectory(this, tr("Select Watch Folder"));
    if (dir.isEmpty())
        return;

    dir = QDir(dir).absolutePath();
    dir = QDir::toNativeSeparators(dir);
    for (int i=0; i<ui_.watchList->count(); ++i)
    {
        const auto item = ui_.watchList->item(i);
        if (item->text() == dir)
        {
            ui_.watchList->setCurrentRow(i);
            return;
        }
    }

    QListWidgetItem* item = new QListWidgetItem;
    item->setText(dir);
    item->setIcon(QIcon("icons:ico_folder.png"));

    ui_.watchList->addItem(item);
    ui_.btnDelWatchFolder->setEnabled(true);
}

void NZBSettings::on_btnDelWatchFolder_clicked()
{
    const auto row = ui_.watchList->currentRow();
    if (row == -1)
        return;

    auto* item = ui_.watchList->takeItem(row);
    ui_.watchList->removeItemWidget(item);
    delete item;

    if (ui_.watchList->count() == 0)
        ui_.btnDelWatchFolder->setEnabled(false);
}

NZBCore::NZBCore() : m_action(DragDropAction::AskForAction)
{
    m_module.PromptForFile = [this] (const QString& file) {

        QFileInfo info(file);
        const auto desc = info.completeBaseName();
        const auto path = info.completeBaseName();
        if (isDuplicate(desc))
            g_win->showWindow();

        if (!passDuplicateCheck(g_win, desc))
            return false;

        if (needAccountUserInput())
            g_win->showWindow();

        const auto acc = selectAccount(g_win, file);
        if (acc == 0)
            return false;

        if (m_module.downloadNzbContents(file, "", path, desc, acc))
            m_module.postProcess(file);
        return true;
    };
}

NZBCore::~NZBCore()
{}


void NZBCore::loadState(app::Settings& settings)
{
    const auto& list  = settings.get("nzb", "watch_folders").toStringList();
    const auto onOff  = settings.get("nzb", "enable_watching").toBool();
    const auto action = settings.get("nzb", "watch_action").toInt();
    const auto dragdrop = settings.get("nzb", "drag_drop_action").toInt();

    m_action = (DragDropAction)dragdrop;

    m_module.setWatchFolders(list);
    m_module.setPostAction((app::NZBCore::PostAction)action);
    m_module.watch(onOff);
}

void NZBCore::saveState(app::Settings& settings)
{
    const auto& list  = m_module.getWatchFolders();
    const auto onOff  = m_module.isEnabled();
    const auto action = m_module.getAction();

    settings.set("nzb", "watch_folders", list);
    settings.set("nzb", "enable_watching", onOff);
    settings.set("nzb", "watch_action", (int)action);
    settings.set("nzb", "drag_drop_action", (int)m_action);
}

SettingsWidget* NZBCore::getSettings()
{
    auto* ptr = new NZBSettings();
    auto& ui  = ptr->ui_;

    const auto& list = m_module.getWatchFolders();
    for (const auto& folder : list)
    {
        QListWidgetItem* item = new QListWidgetItem;
        item->setText(folder);
        item->setIcon(QIcon("icons:ico_folder.png"));
        ui.watchList->addItem(item);
    }

    ui.btnDelWatchFolder->setEnabled(!list.isEmpty());

    const auto onOff = m_module.isEnabled();
    ui.grpWatch->setChecked(onOff);

    const auto action = m_module.getAction();
    if (action == app::NZBCore::PostAction::Rename)
        ui.rdbRename->setChecked(true);
    else if (action == app::NZBCore::PostAction::Delete)
        ui.rdbDelete->setChecked(true);

    if (m_action == DragDropAction::AskForAction)
        ui.rdAskForAction->setChecked(true);
    else if (m_action == DragDropAction::ShowContents)
        ui.rdShowContents->setChecked(true);
    else if (m_action == DragDropAction::DownloadContents)
        ui.rdDownloadContents->setChecked(true);

    return ptr;
}

void NZBCore::applySettings(SettingsWidget* gui)
{
    auto* ptr = dynamic_cast<NZBSettings*>(gui);
    auto& ui  = ptr->ui_;

    QStringList list;
    for (int i=0; i<ui.watchList->count(); ++i)
    {
        const auto* item = ui.watchList->item(i);
        const auto& path = item->text();
        list << path;
    }

    const auto onOff = ui.grpWatch->isChecked();
    m_module.watch(onOff);
    m_module.setWatchFolders(list);

    if (ui.rdbRename->isChecked())
        m_module.setPostAction(app::NZBCore::PostAction::Rename);
    else if (ui.rdbDelete->isChecked())
        m_module.setPostAction(app::NZBCore::PostAction::Delete);

    if (ui.rdAskForAction->isChecked())
        m_action = DragDropAction::AskForAction;
    else if (ui.rdShowContents->isChecked())
        m_action = DragDropAction::ShowContents;
    else if (ui.rdDownloadContents->isChecked())
        m_action = DragDropAction::DownloadContents;
}


void NZBCore::freeSettings(SettingsWidget* s)
{
    delete s;
}

MainWidget* NZBCore::dropFile(const QString& file)
{
    QFileInfo info(file);
    if (info.suffix() != "nzb")
        return nullptr;

    DragDropAction action = m_action;

    if (action == DragDropAction::AskForAction)
    {
        DlgDragDrop dlg(g_win, info.completeBaseName());
        if (dlg.exec() == QDialog::Rejected)
            return nullptr;

        if (dlg.downloadContents())
            action = DragDropAction::DownloadContents;
        else if (dlg.showContents())
            action = DragDropAction::ShowContents;

        if (dlg.rememberSetting())
            m_action = action;
    }

    if (action == DragDropAction::ShowContents)
    {
        auto* widget = new NZBFile;
        widget->openFile(file);
        return widget;
    }
    else if (action == DragDropAction::DownloadContents)
    {
        const auto desc = info.completeBaseName();
        const auto path = info.completeBaseName();
        if (!passDuplicateCheck(g_win, desc))
            return nullptr;

        const auto acc = selectAccount(g_win, info.completeBaseName());
        if (acc == 0)
            return nullptr;

        m_module.downloadNzbContents(file, "", path, desc, acc);
    }
    return nullptr;
}

} // gui
