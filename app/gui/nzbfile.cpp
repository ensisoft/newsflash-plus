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
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QMessageBox>
#  include <QFileInfo>
#include <newsflash/warnpop.h>

#include "nzbfile.h"
#include "mainwindow.h"
#include "../debug.h"
#include "../format.h"
#include "../eventlog.h"
#include "../settings.h"

namespace gui
{

NZBFile::NZBFile()
{
    ui_.setupUi(this);
    ui_.progressBar->setVisible(false);
    ui_.progressBar->setValue(0);
    ui_.progressBar->setRange(0, 0);
    ui_.tableView->setModel(&model_);

    ui_.actionDownload->setEnabled(false);
    ui_.actionBrowse->setEnabled(false);
    ui_.actionClear->setEnabled(false);
    ui_.actionDelete->setEnabled(false);

    model_.on_ready = [&](bool success) 
    {
        ui_.progressBar->setVisible(false);
        ui_.actionDownload->setEnabled(success);
        ui_.actionClear->setEnabled(success);
        ui_.actionDelete->setEnabled(success);
        ui_.actionBrowse->setEnabled(success);        
    };
    DEBUG("NZBFile created");
}

NZBFile::~NZBFile()
{
    DEBUG("NZBFile deleted");
}

void NZBFile::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionOpen);
    menu.addSeparator();
    menu.addAction(ui_.actionDownload);
    menu.addSeparator();
    menu.addAction(ui_.actionClear);
    menu.addSeparator();
    menu.addAction(ui_.actionDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionSettings);
}

void NZBFile::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionOpen);
    bar.addSeparator();
    bar.addAction(ui_.actionDownload);
    bar.addSeparator();
    bar.addAction(ui_.actionClear);
    bar.addSeparator();
    bar.addAction(ui_.actionDelete);
    bar.addSeparator();
    bar.addAction(ui_.actionSettings);
}

void NZBFile::closeWidget()
{
    delete this;
}

void NZBFile::loadState(app::Settings& s)
{
    const bool filenames = s.get("nzb", "show_filename_only", false);

    ui_.chkFilenamesOnly->setChecked(filenames);

    model_.set_show_filenames_only(filenames);
}

bool NZBFile::saveState(app::Settings& s)
{
    s.set("nzb", "show_filename_only", 
        ui_.chkFilenamesOnly->isChecked());
    return true;
}

void NZBFile::open(const QString& nzbfile)
{
    Q_ASSERT(!nzbfile.isEmpty());

    const bool filenames = ui_.chkFilenamesOnly->isChecked();

    model_.set_show_filenames_only(filenames);

    if (model_.load(nzbfile))
    {
        ui_.progressBar->setVisible(true);
        ui_.grpBox->setTitle(nzbfile);
    }
}

void NZBFile::open(const QByteArray& bytes, const QString& desc)
{
    Q_ASSERT(!bytes.isEmpty());
    Q_ASSERT(!desc.isEmpty());

    const bool filenames = ui_.chkFilenamesOnly->isChecked();

    model_.set_show_filenames_only(filenames);

    if (model_.load(bytes, desc))
    {
        ui_.progressBar->setVisible(true);
        ui_.grpBox->setTitle(desc);
    }
}

void NZBFile::on_actionOpen_triggered()
{
    const auto& file = g_win->selectNzbOpenFile();
    if (file.isEmpty())
        return;

    if (model_.load(file))
    {
        ui_.progressBar->setVisible(true);
        ui_.grpBox->setTitle(file);
    }
}

void NZBFile::on_actionDownload_triggered()
{
    downloadSelected("");
}

void NZBFile::on_actionBrowse_triggered()
{
    const auto& folder = g_win->selectDownloadFolder();
    if (folder.isEmpty())
        return;

    downloadSelected(folder);
}

void NZBFile::on_actionClear_triggered()
{
    model_.clear();
    ui_.grpBox->setTitle(tr("NZB File"));
    ui_.actionDownload->setEnabled(false);
    ui_.actionDelete->setEnabled(false);
}

void NZBFile::on_actionDelete_triggered()
{
    auto indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    model_.kill(indices);
}

void NZBFile::on_actionSettings_triggered()
{
    g_win->showSetting("NZB");
}

void NZBFile::on_tableView_customContextMenuRequested(QPoint)
{
    QMenu sub("Download to");
    sub.setIcon(QIcon(":/resource/16x16_ico_png/ico_download.png"));

    const auto& recents = g_win->getRecentPaths();
    for (const auto& recent : recents)
    {
        QAction* action = sub.addAction(QIcon(":/resource/16x16_ico_png/ico_folder.png"), recent);
        QObject::connect(action, SIGNAL(triggered(bool)),
            this, SLOT(downloadToPrevious()));
    }
    sub.addSeparator();
    sub.addAction(ui_.actionBrowse);
    sub.setEnabled(ui_.actionDownload->isEnabled());

    QMenu menu;
    menu.addAction(ui_.actionOpen);
    menu.addSeparator();
    menu.addAction(ui_.actionDownload);
    menu.addMenu(&sub);
    menu.addSeparator();
    menu.addAction(ui_.actionClear);
    menu.addSeparator();
    menu.addAction(ui_.actionDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionSettings);
    menu.exec(QCursor::pos());
}

void NZBFile::on_chkFilenamesOnly_clicked()
{
    const bool on_off = ui_.chkFilenamesOnly->isChecked();

    model_.set_show_filenames_only(on_off);
}

void NZBFile::downloadToPrevious()
{
    const auto* action = qobject_cast<const QAction*>(sender());

    const auto folder = action->text();

    downloadSelected(folder);
}

void NZBFile::downloadSelected(const QString& folder)
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto& filename = ui_.grpBox->title();
    const auto& basename = QFileInfo(filename).completeBaseName();

    const auto acc = g_win->chooseAccount(basename);
    if (!acc)
        return;

    model_.download(indices, acc, folder, basename);
}

} // gui


