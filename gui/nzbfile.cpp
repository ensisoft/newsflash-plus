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
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QMessageBox>
#  include <QFileInfo>
#include "newsflash/warnpop.h"

#include "app/debug.h"
#include "app/format.h"
#include "app/eventlog.h"
#include "app/settings.h"
#include "app/utility.h"
#include "nzbfile.h"
#include "mainwindow.h"
#include "finder.h"
#include "common.h"

namespace gui
{

NZBFile::NZBFile(app::MediaType type) : mModel(type)
{
    mUi.setupUi(this);
    mUi.progressBar->setVisible(false);
    mUi.progressBar->setValue(0);
    mUi.progressBar->setRange(0, 0);
    mUi.tableView->setModel(&mModel);
    mUi.actionDownload->setEnabled(false);
    mUi.actionBrowse->setEnabled(false);

    DEBUG("NZBFile UI created");
}

NZBFile::NZBFile() : NZBFile(app::MediaType::SENTINEL)
{}

NZBFile::~NZBFile()
{
    DEBUG("NZBFile UI deleted");
}

void NZBFile::addActions(QMenu& menu)
{
    menu.addAction(mUi.actionDownload);
}

void NZBFile::addActions(QToolBar& bar)
{
    bar.addAction(mUi.actionDownload);
}

void NZBFile::loadState(app::Settings& settings)
{
    app::loadState("nzbfile", mUi.chkFilenamesOnly, settings);

    on_chkFilenamesOnly_clicked();
}

void NZBFile::saveState(app::Settings& settings)
{
    app::saveState("nzbfile", mUi.chkFilenamesOnly, settings);
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
    const auto& item = mModel.getItem(index);
    if (!caseSensitive)
    {
        auto upper = item.subject.toUpper();
        return upper.indexOf(str) != -1;
    }

    return item.subject.indexOf(str) != -1;
}

bool NZBFile::isMatch(const QRegExp& regex, std::size_t index)
{
    const auto& item = mModel.getItem(index);

    return regex.indexIn(item.subject) != -1;
}

std::size_t NZBFile::numItems() const
{
    return mModel.numItems();
}

std::size_t NZBFile::curItem() const
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return 0;

    const auto& first = indices.first();
    return first.row();
}

void NZBFile::setFound(std::size_t index)
{
    auto* model = mUi.tableView->model();
    auto i = model->index(index, 0);
    mUi.tableView->setCurrentIndex(i);
    mUi.tableView->scrollTo(i);
}

void NZBFile::open(const QString& nzbfile)
{
    mModel.on_ready = [&](bool success)
    {
        mUi.progressBar->setVisible(false);
        mUi.actionDownload->setEnabled(success);
        mUi.actionBrowse->setEnabled(success);
        mUi.tableView->sortByColumn((int)app::NZBFile::Columns::File);
        if (!success)
        {
            QMessageBox::critical(this, "An Error Occurred",
                "The Newzbin file could not be loaded.");
        }
    };

    if (mModel.load(nzbfile))
    {
        mUi.progressBar->setVisible(true);
        mUi.grpBox->setTitle(nzbfile);
    }
}

void NZBFile::open(const QByteArray& bytes, const QString& desc)
{
    mModel.on_ready = [&](bool success)
    {
        mUi.progressBar->setVisible(false);
        mUi.actionDownload->setEnabled(success);
        mUi.actionBrowse->setEnabled(success);
        mUi.tableView->sortByColumn((int)app::NZBFile::Columns::File);
    };

    if (mModel.load(bytes, desc))
    {
        mUi.progressBar->setVisible(true);
        mUi.grpBox->setTitle(desc);
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
    sub.addAction(mUi.actionBrowse);
    sub.setEnabled(mUi.actionDownload->isEnabled());

    QMenu menu;
    menu.addAction(mUi.actionDownload);
    menu.addMenu(&sub);
    menu.exec(QCursor::pos());
}

void NZBFile::on_chkFilenamesOnly_clicked()
{
    const bool on_off = mUi.chkFilenamesOnly->isChecked();

    mModel.setShowFilenamesOnly(on_off);
}

void NZBFile::on_btnSelectAll_clicked()
{
    const QModelIndex parent;
    const QModelIndex topLeft = mModel.index(0, 0, parent);
    const QModelIndex bottomRight = mModel.index(mModel.rowCount(parent)-1,
         mModel.columnCount(parent)-1, parent);
    const QItemSelection selection(topLeft, bottomRight);
    auto* selectionModel = mUi.tableView->selectionModel();
    selectionModel->select(selection, QItemSelectionModel::Select);
}

void NZBFile::on_btnSelectNone_clicked()
{
    mUi.tableView->selectionModel()->clearSelection();
}

void NZBFile::downloadToPrevious()
{
    const auto* action = qobject_cast<const QAction*>(sender());

    const auto folder = action->text();

    downloadSelected(folder);
}

void NZBFile::downloadSelected(const QString& folder)
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto& filename = mUi.grpBox->title();
    const auto& basename = QFileInfo(filename).completeBaseName();

    // todo: should we do duplicate checking here somehow?

    const auto mediatype = mModel.findMediaType();
    const auto byteSize  = mModel.sumDataSizes(indices);
    if (!passSpaceCheck(this, basename, folder, byteSize, byteSize, mediatype))
        return;

    const auto acc = selectAccount(this, basename);
    if (!acc)
        return;

    mModel.downloadSel(indices, acc, folder, basename);
}

} // gui


