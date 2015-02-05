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

#define LOGTAG "nzb"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QFileDialog>
#  include <QtGui/QAction>
#  include <QtGui/QMenu>
#  include <QDir>
#include <newsflash/warnpop.h>
#include "mainwindow.h"
#include "nzbcore.h"
#include "nzbfile.h"
#include "../settings.h"
#include "../debug.h"

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
    if (ui_.rdbMoveProcessed->isChecked())
    {
        const auto& dir = ui_.editDumpFolder->text(); 
        if (dir.isEmpty())
        {
            ui_.editDumpFolder->setFocus();
            return false;
        }
    }
    if (ui_.grpCustomDownloadFolder->isChecked())
    {
        const auto& dir = ui_.editCustomDownloadFolder->text();
        if (dir.isEmpty())
        {
            ui_.editCustomDownloadFolder->setFocus();
            return false;
        }
    }
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

    ui_.watchList->addItem(dir);
    ui_.btnDelWatchFolder->setEnabled(true);
}

void NZBSettings::on_btnDelWatchFolder_clicked()
{
    if (ui_.watchList->count() == 0)
        ui_.btnDelWatchFolder->setEnabled(false);
}

void NZBSettings::on_btnSelectDumpFolder_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this, tr("Select NZB Dump Folder"));
    if (dir.isEmpty())
        return;

    dir = QDir(dir).absolutePath();
    dir = QDir::toNativeSeparators(dir);
    ui_.editDumpFolder->setText(dir);
}

void NZBSettings::on_btnSelectDownloadFolder_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this, tr("Select Custom Download Folder"));
    if (dir.isEmpty())
        return;

    dir = QDir(dir).absolutePath();
    dir = QDir::toNativeSeparators(dir);
    ui_.editCustomDownloadFolder->setText(dir);
}

NZBCore::NZBCore()
{}

NZBCore::~NZBCore()
{}

bool NZBCore::addActions(QMenu& menu)
{
    auto* download = menu.addAction(QIcon("icons:ico_download.png"),
        tr("Download NZB"));
    auto* display  = menu.addAction(QIcon("icons:ico_open_nzb.png"),
        tr("Open NZB"));

    display->setShortcut(QKeySequence::Open);

    QObject::connect(download, SIGNAL(triggered()), this, SLOT(downloadTriggered()));
    QObject::connect(display,  SIGNAL(triggered()), this, SLOT(displayTriggered()));
    return true;
}

void NZBCore::loadState(app::Settings& s)
{}

bool NZBCore::saveState(app::Settings& s)
{
    return true;
}

SettingsWidget* NZBCore::getSettings()
{
    auto* ptr = new NZBSettings();
    auto& ui  = ptr->ui_;

    // const auto& list = s.get("nzb", "watched_folders").toStringList();

    // ui.btnDelWatchFolder->setEnabled(!list.isEmpty());

    // for (int i=0; i<list.size(); ++i)
    //     ui.watchList->addItem(list[i]);

    // const auto enable_watch = s.get("nzb", "enable_watching", false);
    // const auto dumps = s.get("nzb", "nzb_dump_folder", "");
    // const auto downloads = s.get("nzb", "download_folder", "");
    // const auto enable_download_folder = s.get("nzb", "enable_custom_download_folder", false);

    // ui.editDumpFolder->setText(dumps);
    // ui.editCustomDownloadFolder->setText(downloads);
    // ui.grpAutoDownload->setChecked(enable_watch);    
    // ui.grpCustomDownloadFolder->setChecked(enable_download_folder);
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

    const auto enable_watch = ui.grpAutoDownload->isChecked();
    const auto nzb_dump_folder = ui.editDumpFolder->text();
    const auto download_folder = ui.editCustomDownloadFolder->text();
    const auto enable_dowload_folder = ui.grpCustomDownloadFolder->isChecked();

    // backend.set("nzb", "watched_folders", list);
    // backend.set("nzb", "enable_watching", enable_watch);
    // backend.set("nzb", "nzb_dump_folder", nzb_dump_folder);
    // backend.set("nzb", "download_folder", download_folder);
    // backend.set("nzb", "enable_custom_download_folder", enable_dowload_folder);

    module_.set_watch_folders(list);
    module_.watch(enable_watch);
}


void NZBCore::freeSettings(SettingsWidget* s)
{
    delete s;
}

void NZBCore::dropFile(const QString& file)
{
    QFileInfo info(file);
    if (!info.exists())
        return;

    if (info.suffix() != ".nzb")
        return;

    
}


void NZBCore::downloadTriggered()
{
    const auto nzb = g_win->selectNzbOpenFile();
    if (nzb.isEmpty())
        return;

    DEBUG("download");
}

void NZBCore::displayTriggered()
{
    const auto nzb = g_win->selectNzbOpenFile();
    if (nzb.isEmpty())
        return;

    auto* file = new NZBFile();
    g_win->attach(file, true);

    file->open(nzb);
}




} // gui
