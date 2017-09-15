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

#define LOGTAG "news"

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QtGui/QMessageBox>
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#  include <QtGui/QPixmap>
#  include <QtGui/QKeyEvent>
#  include <QFile>
#  include <QFileInfo>
#  include <QDir>
#include "newsflash/warnpop.h"
#include <limits>
#include "newsgroup.h"
#include "mainwindow.h"
#include "dlgfilter.h"
#include "dlgvolumes.h"
#include "common.h"
#include "app/debug.h"
#include "app/format.h"
#include "app/utility.h"
#include "app/engine.h"
#include "app/fileinfo.h"
#include "app/types.h"

namespace gui
{

using FileType = app::NewsGroup::FileType;
using FileFlag = app::NewsGroup::FileFlag;

NewsGroup::NewsGroup(quint32 acc, QString path, QString name) : account_(acc), path_(path), name_(name)
{
    using Cols = app::NewsGroup::Columns;

    ui_.setupUi(this);
    ui_.tableView->setModel(&model_);
    ui_.grpView->setTitle(app::toString("%1 (%2 / %3)", name, app::count {0}, app::count{0}));
    ui_.tableView->setColumnWidth((int)Cols::DownloadFlag, 32);
    ui_.tableView->setColumnWidth((int)Cols::BrokenFlag, 32);
    ui_.tableView->setColumnWidth((int)Cols::BookmarkFlag, 32);
    ui_.tableView->installEventFilter(this);
    ui_.progressBar->setVisible(false);
    ui_.loader->setVisible(false);
    ui_.actionRefresh->setShortcut(QKeySequence::Refresh);
    ui_.actionStop->setEnabled(false);
    ui_.actionDownload->setEnabled(false);
    ui_.actionShowNone->setChecked(true);
    ui_.actionShowAudio->setChecked(true);
    ui_.actionShowVideo->setChecked(true);
    ui_.actionShowImage->setChecked(true);
    ui_.actionShowText->setChecked(true);
    ui_.actionShowArchive->setChecked(true);
    ui_.actionShowParity->setChecked(true);
    ui_.actionShowDocument->setChecked(true);
    ui_.actionShowOther->setChecked(true);
    ui_.actionShowBroken->setChecked(true);
    ui_.actionShowDeleted->setChecked(true);

    QObject::connect(app::g_engine, SIGNAL(newHeaderInfoAvailable(const QString&, quint64, quint64)),
        this, SLOT(newHeaderInfoAvailable(const QString&, quint64, quint64)));
    QObject::connect(app::g_engine, SIGNAL(newHeaderDataAvailable(const QString&, const QString&)),
        this, SLOT(newHeaderDataAvailable(const QString&, const QString&)));
    QObject::connect(app::g_engine, SIGNAL(updateCompleted(const app::HeaderInfo&)),
        this, SLOT(updateCompleted(const app::HeaderInfo&)));

    QObject::connect(&model_, SIGNAL(modelAboutToBeReset()),
        this, SLOT(modelBegReset()));
    QObject::connect(&model_, SIGNAL(modelReset()),
        this, SLOT(modelEndReset()));
    QObject::connect(&model_, SIGNAL(layoutChanged()),
        this, SLOT(modelLayoutChanged()));

    QObject::connect(ui_.tableView->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));

    setWindowTitle(name);

    model_.onLoadBegin = [this](std::size_t curBlock, std::size_t numBlocks) {
        ui_.loader->setVisible(true);
        ui_.btnLoadMore->setEnabled(false);
    };

    model_.onLoadProgress = [this] (std::size_t curItem, std::size_t numItems) {
        ui_.loader->setMaximum(numItems);
        ui_.loader->setValue(curItem);
    };
    model_.onLoadComplete = [this](std::size_t curBlock, std::size_t numBlocks) {
        ui_.loader->setVisible(false);
        const auto numLoaded = model_.numBlocksLoaded();
        const auto numTotal  = model_.numBlocksAvail();
        ui_.btnLoadMore->setEnabled(numLoaded != numTotal);
        ui_.btnLoadMore->setText(
            tr("Load more headers ... (%1/%2)").arg(numLoaded).arg(numTotal));
    };
    model_.onKilled = [this] {
        ui_.actionStop->setEnabled(false);
        ui_.progressBar->setVisible(false);
        ui_.actionRefresh->setEnabled(true);
    };

    filter_.minSize  = 0;
    filter_.maxSize  = std::numeric_limits<quint32>::max() + 1ull;
    filter_.bMinSize = false;
    filter_.bMaxSize = false;
    filter_.minDays  = 0;
    filter_.maxDays  = 12527;
    filter_.bMinDays = false;
    filter_.bMaxDays = false;
    filter_.sizeUnits = DlgFilter::Unit::MB;
    filter_.matchStringCaseSensitive = true;
    model_.setSizeFilter(filter_.minSize, filter_.maxSize);
    model_.setDateFilter(filter_.minDays, filter_.maxDays);

    DEBUG("NewsGroup GUI created");
}

NewsGroup::~NewsGroup()
{
    DEBUG("Newsgroup GUI deleted");
}

void NewsGroup::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionRefresh);
    bar.addSeparator();
    bar.addAction(ui_.actionDownload);
    bar.addSeparator();
    bar.addAction(ui_.actionHeaders);
    bar.addSeparator();
    bar.addAction(ui_.actionFilter);
    bar.addSeparator();
    bar.addAction(ui_.actionBookmarkPrev);
    bar.addAction(ui_.actionBookmark);
    bar.addAction(ui_.actionBookmarkNext);
    bar.addSeparator();
    bar.addAction(ui_.actionDelete);
    bar.addSeparator();
    bar.addAction(ui_.actionStop);
}

void NewsGroup::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();
    menu.addAction(ui_.actionDownload);
    menu.addSeparator();
    menu.addAction(ui_.actionHeaders);
    menu.addSeparator();
    menu.addAction(ui_.actionFilter);
    menu.addSeparator();
    menu.addAction(ui_.actionBookmarkPrev);
    menu.addAction(ui_.actionBookmark);
    menu.addAction(ui_.actionBookmarkNext);
    menu.addSeparator();
    menu.addAction(ui_.actionDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionStop);
}

void NewsGroup::loadState(app::Settings& settings)
{
    ui_.tableView->setSortingEnabled(false);

    app::loadTableLayout("newsgroup", ui_.tableView, settings);

    ui_.tableView->setSortingEnabled(true);
    ui_.tableView->sortByColumn((int)app::NewsGroup::Columns::Age,
        Qt::DescendingOrder);

    app::loadState("newsgroup", ui_.actionShowNone, settings);
    app::loadState("newsgroup", ui_.actionShowAudio, settings);
    app::loadState("newsgroup", ui_.actionShowVideo, settings);
    app::loadState("newsgroup", ui_.actionShowImage, settings);
    app::loadState("newsgroup", ui_.actionShowText, settings);
    app::loadState("newsgroup", ui_.actionShowArchive, settings);
    app::loadState("newsgroup", ui_.actionShowParity, settings);
    app::loadState("newsgroup", ui_.actionShowDocument, settings);
    app::loadState("newsgroup", ui_.actionShowOther, settings);
    app::loadState("newsgroup", ui_.actionShowBroken, settings);
    app::loadState("newsgroup", ui_.actionShowDeleted, settings);
}

void NewsGroup::saveState(app::Settings& settings)
{
    app::saveTableLayout("newsgroup", ui_.tableView, settings);
    app::saveState("newsgroup", ui_.actionShowNone, settings);
    app::saveState("newsgroup", ui_.actionShowAudio, settings);
    app::saveState("newsgroup", ui_.actionShowVideo, settings);
    app::saveState("newsgroup", ui_.actionShowImage, settings);
    app::saveState("newsgroup", ui_.actionShowText, settings);
    app::saveState("newsgroup", ui_.actionShowArchive, settings);
    app::saveState("newsgroup", ui_.actionShowParity, settings);
    app::saveState("newsgroup", ui_.actionShowDocument, settings);
    app::saveState("newsgroup", ui_.actionShowOther, settings);
    app::saveState("newsgroup", ui_.actionShowBroken, settings);
    app::saveState("newsgroup", ui_.actionShowDeleted, settings);
}

MainWidget::info NewsGroup::getInformation() const
{
    return {"group.html", false};
}

Finder* NewsGroup::getFinder()
{
    return this;
}

bool NewsGroup::isMatch(const QString& str, std::size_t index, bool caseSensitive)
{
    const auto& article = model_.getArticle(index);
    const auto& subject = article.subject();
    const auto& latin   = QString::fromLatin1(subject.c_str(), subject.size());
    if (!caseSensitive)
    {
        auto upper = latin.toUpper();
        if (upper.indexOf(str) != -1)
            return true;
    }
    else
    {
        if (latin.indexOf(str) != -1)
            return true;
    }
    return false;
}

bool NewsGroup::isMatch(const QRegExp& reg, std::size_t index)
{
    const auto& article = model_.getArticle(index);
    const auto& subject = article.subject();
    const auto& latin   = QString::fromLatin1(subject.c_str(), subject.size());
    if (reg.indexIn(latin) != -1)
        return true;
    return false;
}

std::size_t NewsGroup::numItems() const
{
    return model_.numShown();
}

std::size_t NewsGroup::curItem() const
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return 0;

    const auto& first = indices.first();
    return first.row();
}

void NewsGroup::setFound(std::size_t index)
{
    auto* model = ui_.tableView->model();
    auto i = model->index(index, 0);
    ui_.tableView->setCurrentIndex(i);
    ui_.tableView->scrollTo(i);
}

void NewsGroup::load()
{
    try
    {
        if (!model_.init(path_, name_))
        {
            model_.refresh(account_, path_, name_);
            ui_.progressBar->setVisible(true);
            ui_.progressBar->setMaximum(0),
            ui_.progressBar->setMinimum(0);
            ui_.actionStop->setEnabled(true);
            ui_.actionRefresh->setEnabled(false);
            ui_.btnLoadMore->setVisible(false);
        }
        else
        {
            model_.load(0);
        }
    }
    catch (const std::exception& e)
    {
        ui_.progressBar->setVisible(false);
        ui_.loader->setVisible(false);
        QMessageBox::critical(this, name_,
            tr("Unable to load the newsgroup data.\n%1").arg(app::widen(e.what())));
    }
}

void NewsGroup::deleteData(quint32 account, QString path, QString group)
{
    app::NewsGroup::deleteData(account, std::move(path), std::move(group));
}

void NewsGroup::on_actionShowNone_changed()
{
    model_.setTypeFilter(FileType::none, ui_.actionShowNone->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionShowAudio_changed()
{
    model_.setTypeFilter(FileType::audio, ui_.actionShowAudio->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionShowVideo_changed()
{
    model_.setTypeFilter(FileType::video, ui_.actionShowVideo->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionShowImage_changed()
{
    model_.setTypeFilter(FileType::image, ui_.actionShowImage->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionShowText_changed()
{
    model_.setTypeFilter(FileType::text, ui_.actionShowText->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionShowArchive_changed()
{
    model_.setTypeFilter(FileType::archive, ui_.actionShowArchive->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionShowParity_changed()
{
    model_.setTypeFilter(FileType::parity, ui_.actionShowParity->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionShowDocument_changed()
{
    model_.setTypeFilter(FileType::document, ui_.actionShowDocument->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionShowOther_changed()
{
    model_.setTypeFilter(FileType::other, ui_.actionShowOther->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionShowBroken_changed()
{
    model_.setFlagFilter(FileFlag::broken, ui_.actionShowBroken->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionShowDeleted_changed()
{
    model_.setFlagFilter(FileFlag::deleted, ui_.actionShowDeleted->isChecked());
    model_.applyFilter();
}

void NewsGroup::on_actionDelete_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    model_.killSelected(indices);
}

void NewsGroup::on_actionHeaders_triggered()
{
    DlgVolumes dlg(this, model_);
    dlg.exec();
}

void NewsGroup::on_actionRefresh_triggered()
{
    model_.refresh(account_, path_, name_);
    ui_.actionRefresh->setEnabled(false);
    ui_.actionStop->setEnabled(true);
    ui_.progressBar->setMaximum(0);
    ui_.progressBar->setMinimum(0);
    ui_.progressBar->setVisible(true);
}

void NewsGroup::on_actionFilter_triggered()
{
    DlgFilter::Params filter = filter_;

    DlgFilter dlg(this, filter);
    dlg.applyFilter = [this](quint32 minDays, quint32 maxDays, quint64 minSize, quint64 maxSize,
        const QString& matchString, bool matchStringCaseSensitive) {
        model_.setSizeFilter(minSize, maxSize);
        model_.setDateFilter(minDays, maxDays);
        model_.setStringFilter(matchString, matchStringCaseSensitive);
        model_.applyFilter();
    };
    if (dlg.exec() == QDialog::Accepted)
    {
        filter_ = filter;
        if (dlg.isApplied())
            return;
    }

    model_.setSizeFilter(filter_.minSize, filter_.maxSize);
    model_.setDateFilter(filter_.minDays, filter_.maxDays);
    model_.setStringFilter(filter_.matchString, filter_.matchStringCaseSensitive);
    model_.applyFilter();
}

void NewsGroup::on_actionStop_triggered()
{
    model_.stop();
    ui_.actionStop->setEnabled(false);
    ui_.progressBar->setVisible(false);
    ui_.actionRefresh->setEnabled(true);
}

void NewsGroup::on_actionDownload_triggered()
{
    downloadSelected("");
}

void NewsGroup::on_actionBrowse_triggered()
{
    const auto& folder = g_win->selectDownloadFolder();
    if (folder.isEmpty())
        return;

    downloadSelected(folder);
}

void NewsGroup::on_actionBookmark_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    model_.bookmark(indices);
}

void NewsGroup::on_actionBookmarkPrev_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    int start = indices.isEmpty() ? 0
        : indices[0].row() - 1;

    const auto numItems = model_.numShown();
    for (std::size_t i=0; i<numItems; ++i, --start)
    {
        if (start < 0)
            start = numItems - 1;

        const auto& item = model_.getArticle(start);
        if ( item.is_bookmarked())
        {
            setFound(start);
            break;
        }
    }
}

void NewsGroup::on_actionBookmarkNext_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    const auto start = indices.isEmpty() ? 0
        : indices[0].row() + 1;

    const auto numItems = model_.numShown();
    for (std::size_t i=0; i<numItems; ++i)
    {
        const auto index = (start + i) % numItems;
        const auto& item = model_.getArticle(index);
        if (item.is_bookmarked())
        {
            setFound(index);
            break;
        }
    }
}

void NewsGroup::on_btnLoadMore_clicked()
{
    ui_.loader->setVisible(true);
    ui_.btnLoadMore->setEnabled(false);

    model_.load();

    ui_.loader->setVisible(false);

}

void NewsGroup::on_tableView_customContextMenuRequested(QPoint p)
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

    QMenu menu(this);
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();
    menu.addAction(ui_.actionHeaders);
    menu.addSeparator();
    menu.addAction(ui_.actionDownload);
    menu.addMenu(&sub);
    menu.addSeparator();
    menu.addAction(ui_.actionFilter);


    QMenu showType("Filter by type", this);
    showType.setIcon(QIcon("icons:ico_filter.png"));
    showType.addAction(ui_.actionShowNone);
    showType.addAction(ui_.actionShowAudio);
    showType.addAction(ui_.actionShowVideo);
    showType.addAction(ui_.actionShowImage);
    showType.addAction(ui_.actionShowText);
    showType.addAction(ui_.actionShowArchive);
    showType.addAction(ui_.actionShowParity);
    showType.addAction(ui_.actionShowDocument);
    showType.addAction(ui_.actionShowOther);

    QMenu showFlag("Filter by status", this);
    showFlag.setIcon(QIcon("icons:ico_filter.png"));
    showFlag.addAction(ui_.actionShowBroken);
    showFlag.addAction(ui_.actionShowDeleted);

    menu.addMenu(&showType);
    menu.addMenu(&showFlag);
    menu.addSeparator();
    menu.addAction(ui_.actionBookmarkPrev);
    menu.addAction(ui_.actionBookmark);
    menu.addAction(ui_.actionBookmarkNext);
    menu.addSeparator();
    menu.addAction(ui_.actionDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionStop);
    menu.exec(QCursor::pos());
}

void NewsGroup::on_tableView_doubleClicked(const QModelIndex&)
{
    downloadSelected("");
}

void NewsGroup::downloadToPrevious()
{
    const auto* action = qobject_cast<const QAction*>(sender());
    const auto folder  = action->text();
    downloadSelected(folder);
}

void NewsGroup::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    // update the selection bits in the index items.

    // current selection is deselected.
    model_.select(selection_, false);

    // grab new items.
    selection_ = ui_.tableView->selectionModel()->selectedRows();

    // update (store) the item bits.
    model_.select(selection_, true);


    // store the currently selected list of items
    // so we can restore it later.
    //model_.select(deselected.indexes(), false);
    //model_.select(selected.indexes(), true);
    //const auto empty = deselected.empty();

    const auto empty = selection_.empty();

    ui_.actionDownload->setEnabled(!empty);
    ui_.actionBookmark->setEnabled(!empty);
    ui_.actionDelete->setEnabled(!empty);
}

void NewsGroup::modelBegReset()
{
    // intentationally empty placeholder.
}

void NewsGroup::modelEndReset()
{
    // the model was reset... need to scan the model in order to restore the
    // current selection on the UI

    QModelIndexList list;
    model_.scanSelected(list);

    if (!list.empty())
    {
        QItemSelection sel;
        for (const auto& i : list)
            sel.select(i, i);

        auto* model = ui_.tableView->selectionModel();
        model->blockSignals(true);
        model->select(sel,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        model->blockSignals(false);
    }

    selection_ = list;

    const auto numShown = model_.numShown();
    const auto numItems = model_.numItems();
    ui_.grpView->setTitle(app::toString("%1 (%2 / %3)", name_, app::count{numShown}, app::count{numItems}));

    if (!selection_.empty())
    {
        const auto first = selection_[0];
        ui_.tableView->scrollTo(first);
    }
}

void NewsGroup::modelLayoutChanged()
{
    // just forward this.
    modelEndReset();
}

void NewsGroup::newHeaderInfoAvailable(const QString& group, quint64 numLocal, quint64 numRemote)
{
    if (group != name_) return;

    const auto max = std::numeric_limits<int>::max();
    if(numRemote < max)
    {
        ui_.progressBar->setMaximum(numRemote);
        ui_.progressBar->setValue(numLocal);
    }
    else
    {
        const auto d = double(numLocal) / double(numRemote);
        ui_.progressBar->setMaximum(max);
        ui_.progressBar->setValue(d * max);
    }
}

void NewsGroup::newHeaderDataAvailable(const QString& group, const QString& file)
{
    if (file.indexOf(app::joinPath(path_, name_)) != 0)
        return;

    const auto numAvail  = model_.numBlocksAvail();
    const auto numLoaded = model_.numBlocksLoaded();
    ui_.btnLoadMore->setEnabled(numAvail != numLoaded);
    ui_.btnLoadMore->setVisible(true);
    ui_.btnLoadMore->setText(
        tr("Load more headers ... (%1/%2)").arg(numLoaded).arg(numAvail));
}

void NewsGroup::updateCompleted(const app::HeaderInfo& info)
{
    if (info.groupName != name_ ||
        info.groupPath != path_) return;

    ui_.progressBar->setVisible(false);
    ui_.actionStop->setEnabled(false);
    ui_.actionRefresh->setEnabled(true);
}


void NewsGroup::downloadSelected(const QString& folder)
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    Q_ASSERT(account_);

    // we don't need to ask for the account here since
    // the data is specific to an account (because we're using article numbers)
    // and we already know the account.
    //
    // Also we're not doing the duplicate checking since the user can
    // already see what has been downloaded in this view before
    // and because we don't have a bullet proof way of grouping
    // selected items into batches with well defined names.
    //
    // However we need to and should do the space checking.
    const auto media    = model_.findMediaType();
    const auto byteSize = model_.sumDataSizes(indices);
    const auto desc = model_.suggestName(indices);
    if (!passSpaceCheck(this, desc, folder, byteSize, byteSize, media))
        return;

    model_.download(indices, account_, folder);
}

bool NewsGroup::eventFilter(QObject* recv, QEvent* event)
{
    Q_ASSERT(recv == ui_.tableView);

    if (event->type() == QEvent::KeyPress)
    {
        const auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Space)
        {
            on_btnLoadMore_clicked();
        }
        else if (key->key() == Qt::Key_Escape)
        {
            ui_.tableView->clearSelection();
        }
    }
    return false;
}

} // gui
