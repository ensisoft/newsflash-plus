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
}

NZBCore::NZBCore()
{}

NZBCore::~NZBCore()
{}


void NZBCore::loadState(app::Settings& s)
{}
    
void NZBCore::saveState(app::Settings& s)
{
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


    module_.set_watch_folders(list);
    module_.watch(enable_watch);
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

    auto* widget = new NZBFile;
    widget->open(file);

    return widget;
}

MainWidget* NZBCore::openFile(const QString& file)
{
    QFileInfo info(file);
    if (info.suffix() != "nzb")
        return nullptr;

    auto* widget = new NZBFile;
    widget->open(file);

    return widget;
}

} // gui
