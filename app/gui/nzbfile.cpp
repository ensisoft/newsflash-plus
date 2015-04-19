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
#include "finder.h"
#include "../debug.h"
#include "../format.h"
#include "../eventlog.h"
#include "../settings.h"
#include "../utility.h"

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

    DEBUG("NZBFile UI created");
}

NZBFile::~NZBFile()
{
    DEBUG("NZBFile UI deleted");
}

void NZBFile::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionDownload);
}

void NZBFile::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionDownload);
}

void NZBFile::loadState(app::Settings& settings)
{
    app::loadState("nzbfile", ui_.chkFilenamesOnly, settings);

    on_chkFilenamesOnly_clicked();
}

void NZBFile::saveState(app::Settings& settings)
{
    app::saveState("nzbfile", ui_.chkFilenamesOnly, settings);
}

MainWidget::info NZBFile::getInformation() const 
{
    return {"nzb.html", false};     
}

Finder* NZBFile::getFinder() 
{
    return this;
}

bool NZBFile::isMatch(const QString& str, std::size_t index, bool caseSensitive)
{
    const auto& item = model_.getItem(index);
    if (!caseSensitive)
    {
        auto upper = item.subject.toUpper();
        return upper.indexOf(str) != -1;
    }

    return item.subject.indexOf(str) != -1;
}

bool NZBFile::isMatch(const QRegExp& regex, std::size_t index)
{
    const auto& item = model_.getItem(index);

    return regex.indexIn(item.subject) != -1;
}

std::size_t NZBFile::numItems() const 
{
    return model_.numItems();
}

std::size_t NZBFile::curItem() const 
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return 0;

    const auto& first = indices.first();
    return first.row();
}

void NZBFile::setFound(std::size_t index)
{
    auto* model = ui_.tableView->model();
    auto i = model->index(index, 0);
    ui_.tableView->setCurrentIndex(i);
    ui_.tableView->scrollTo(i);
}

void NZBFile::open(const QString& nzbfile)
{
    model_.on_ready = [&](bool success) 
    {
        ui_.progressBar->setVisible(false);
        ui_.actionDownload->setEnabled(success);
        ui_.actionBrowse->setEnabled(success);     
        ui_.tableView->sortByColumn((int)app::NZBFile::Columns::File);
        if (!success)
        {
            QMessageBox::critical(this, "An Error Occurred",
                "The Newzbin file could not be loaded.");
        }           
    };

    if (model_.load(nzbfile))
    {
        ui_.progressBar->setVisible(true);
        ui_.grpBox->setTitle(nzbfile);
    }
}

void NZBFile::open(const QByteArray& bytes, const QString& desc)
{
    model_.on_ready = [&](bool success) 
    {
        ui_.progressBar->setVisible(false);
        ui_.actionDownload->setEnabled(success);
        ui_.actionBrowse->setEnabled(success);        
        ui_.tableView->sortByColumn((int)app::NZBFile::Columns::File);
    };

    if (model_.load(bytes, desc))
    {
        ui_.progressBar->setVisible(true);
        ui_.grpBox->setTitle(desc);
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

void NZBFile::on_tableView_customContextMenuRequested(QPoint)
{
    QMenu sub("Download to");
    sub.setIcon(QIcon("icons:ico_download.png"));

    const auto& recents = g_win->getRecentPaths();
    for (const auto& recent : recents)
    {
        QAction* action = sub.addAction(QIcon("icons:ico_folder.png"), recent);
        QObject::connect(action, SIGNAL(triggered(bool)),
            this, SLOT(downloadToPrevious()));
    }
    sub.addSeparator();
    sub.addAction(ui_.actionBrowse);
    sub.setEnabled(ui_.actionDownload->isEnabled());

    QMenu menu;
    menu.addAction(ui_.actionDownload);
    menu.addMenu(&sub);
    menu.exec(QCursor::pos());
}

void NZBFile::on_chkFilenamesOnly_clicked()
{
    const bool on_off = ui_.chkFilenamesOnly->isChecked();

    model_.setShowFilenamesOnly(on_off);
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


