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
#  include <QMessageBox>
#  include <QToolBar>
#  include <QMenu>
#  include <QPixmap>
#  include <QKeyEvent>
#  include <QFile>
#  include <QFileInfo>
#  include <QDir>
#include "newsflash/warnpop.h"

#include <limits>

#include "app/eventlog.h"
#include "app/debug.h"
#include "app/format.h"
#include "app/utility.h"
#include "app/engine.h"
#include "app/fileinfo.h"
#include "app/types.h"

#include "newsgroup.h"
#include "mainwindow.h"
#include "dlgfilter.h"
#include "dlgvolumes.h"
#include "common.h"

namespace gui
{

using FileType = app::NewsGroup::FileType;
using FileFlag = app::NewsGroup::FileFlag;

NewsGroup::NewsGroup()
{
    using Cols = app::NewsGroup::Columns;

    mUi.setupUi(this);
    mUi.tableView->setModel(&mModel);
    mUi.tableView->setColumnWidth((int)Cols::DownloadFlag, 32);
    mUi.tableView->setColumnWidth((int)Cols::BrokenFlag, 32);
    mUi.tableView->setColumnWidth((int)Cols::BookmarkFlag, 32);
    mUi.tableView->installEventFilter(this);
    mUi.progressBar->setVisible(false);
    mUi.loader->setVisible(false);
    mUi.actionRefresh->setShortcut(QKeySequence::Refresh);
    mUi.actionStop->setEnabled(false);
    mUi.actionDownload->setEnabled(false);
    mUi.actionShowNone->setChecked(true);
    mUi.actionShowAudio->setChecked(true);
    mUi.actionShowVideo->setChecked(true);
    mUi.actionShowImage->setChecked(true);
    mUi.actionShowText->setChecked(true);
    mUi.actionShowArchive->setChecked(true);
    mUi.actionShowParity->setChecked(true);
    mUi.actionShowDocument->setChecked(true);
    mUi.actionShowOther->setChecked(true);
    mUi.actionShowBroken->setChecked(true);
    mUi.actionShowDeleted->setChecked(true);

    QObject::connect(app::g_engine, SIGNAL(newHeaderInfoAvailable(const app::HeaderUpdateInfo&)),
        this, SLOT(newHeaderInfoAvailable(const app::HeaderUpdateInfo&)));
    QObject::connect(app::g_engine, SIGNAL(updateCompleted(const app::HeaderInfo&)),
        this, SLOT(updateCompleted(const app::HeaderInfo&)));

    QObject::connect(&mModel, SIGNAL(modelAboutToBeReset()),
        this, SLOT(modelBegReset()));
    QObject::connect(&mModel, SIGNAL(modelReset()),
        this, SLOT(modelEndReset()));
    QObject::connect(&mModel, SIGNAL(layoutChanged()),
        this, SLOT(modelLayoutChanged()));

    QObject::connect(mUi.tableView->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));

    mModel.onLoadBegin = [this](std::size_t curBlock, std::size_t numBlocks) {
        mUi.loader->setVisible(true);
        mUi.btnLoadMore->setEnabled(false);
    };

    mModel.onLoadProgress = [this] (std::size_t curItem, std::size_t numItems) {
        mUi.loader->setMaximum(numItems);
        mUi.loader->setValue(curItem);
    };
    mModel.onLoadComplete = [this](std::size_t curBlock, std::size_t numBlocks) {
        mUi.loader->setVisible(false);
        const auto numLoaded = mModel.numBlocksLoaded();
        const auto numTotal  = mModel.numBlocksAvail();
        mUi.btnLoadMore->setEnabled(numLoaded != numTotal);
        mUi.btnLoadMore->setText(
            tr("Load more ... (%1/%2)").arg(numLoaded).arg(numTotal));
    };
    mModel.onKilled = [this] {
        mUi.actionStop->setEnabled(false);
        mUi.progressBar->setVisible(false);
        mUi.actionRefresh->setEnabled(true);
    };

    mFilterParams.minSize  = 0;
    mFilterParams.maxSize  = std::numeric_limits<quint32>::max() + 1ull;
    mFilterParams.bMinSize = false;
    mFilterParams.bMaxSize = false;
    mFilterParams.minDays  = 0;
    mFilterParams.maxDays  = 12527;
    mFilterParams.bMinDays = false;
    mFilterParams.bMaxDays = false;
    mFilterParams.sizeUnits = DlgFilter::Unit::MB;
    mFilterParams.matchStringCaseSensitive = true;
    mModel.setSizeFilter(mFilterParams.minSize, mFilterParams.maxSize);
    mModel.setDateFilter(mFilterParams.minDays, mFilterParams.maxDays);

    DEBUG("NewsGroup GUI created");
}
NewsGroup::NewsGroup(quint32 acc, QString path, QString name) : NewsGroup()
{
    mAccountId = acc;
    mGroupPath = path;
    mGroupName = name;

    mUi.grpView->setTitle(app::toString("%1 (%2 / %3)", name, app::count {0}, app::count{0}));
    setWindowTitle(name);

    DEBUG("NewsGroup GUI created");
}

NewsGroup::~NewsGroup()
{
    saveState();

    DEBUG("Newsgroup GUI deleted");
}

void NewsGroup::addActions(QToolBar& bar)
{
    bar.addAction(mUi.actionRefresh);
    bar.addAction(mUi.actionStop);
    bar.addSeparator();
    bar.addAction(mUi.actionDownload);
    bar.addSeparator();
    bar.addAction(mUi.actionHeaders);
    bar.addSeparator();
    bar.addAction(mUi.actionFilter);
    bar.addSeparator();
    bar.addAction(mUi.actionBookmarkPrev);
    bar.addAction(mUi.actionBookmark);
    bar.addAction(mUi.actionBookmarkNext);
    bar.addSeparator();
    bar.addAction(mUi.actionDelete);
}

void NewsGroup::addActions(QMenu& menu)
{
    menu.addAction(mUi.actionRefresh);
    menu.addAction(mUi.actionStop);
    menu.addSeparator();
    menu.addAction(mUi.actionDownload);
    menu.addSeparator();
    menu.addAction(mUi.actionHeaders);
    menu.addSeparator();
    menu.addAction(mUi.actionFilter);
    menu.addSeparator();
    menu.addAction(mUi.actionBookmarkPrev);
    menu.addAction(mUi.actionBookmark);
    menu.addAction(mUi.actionBookmarkNext);
    menu.addSeparator();
    menu.addAction(mUi.actionDelete);
}

void NewsGroup::loadState()
{
    // set the default sorting to Ascending age, i.e newest items at the
    // top of the list and older items at the bottom.
    // loadTableLayout below will restore the previous sorting order
    // that was used by the user.
    mUi.tableView->sortByColumn((int)app::NewsGroup::Columns::Age, Qt::AscendingOrder);

    const auto& settingsPath = app::joinPath(mGroupPath, mGroupName);
    const auto& settingsFile = app::joinPath(settingsPath, mGroupName + ".json");
    if (!QFile::exists(settingsFile))
        return;

    app::Settings settings;
    const auto err = settings.load(settingsFile);
    if (err != QFile::NoError)
    {
        ERROR("Failed to read settings %1, %2", settingsFile, err);
        return;
    }

    app::loadTableLayout("newsgroup", mUi.tableView, settings);
    app::loadState("newsgroup", mUi.actionShowNone, settings);
    app::loadState("newsgroup", mUi.actionShowAudio, settings);
    app::loadState("newsgroup", mUi.actionShowVideo, settings);
    app::loadState("newsgroup", mUi.actionShowImage, settings);
    app::loadState("newsgroup", mUi.actionShowText, settings);
    app::loadState("newsgroup", mUi.actionShowArchive, settings);
    app::loadState("newsgroup", mUi.actionShowParity, settings);
    app::loadState("newsgroup", mUi.actionShowDocument, settings);
    app::loadState("newsgroup", mUi.actionShowOther, settings);
    app::loadState("newsgroup", mUi.actionShowBroken, settings);
    app::loadState("newsgroup", mUi.actionShowDeleted, settings);
    app::loadState("newsgroup", mUi.chkRefreshAutomatically, settings);
}

void NewsGroup::saveState() const
{
    app::Settings settings;
    app::saveTableLayout("newsgroup", mUi.tableView, settings);
    app::saveState("newsgroup", mUi.actionShowNone, settings);
    app::saveState("newsgroup", mUi.actionShowAudio, settings);
    app::saveState("newsgroup", mUi.actionShowVideo, settings);
    app::saveState("newsgroup", mUi.actionShowImage, settings);
    app::saveState("newsgroup", mUi.actionShowText, settings);
    app::saveState("newsgroup", mUi.actionShowArchive, settings);
    app::saveState("newsgroup", mUi.actionShowParity, settings);
    app::saveState("newsgroup", mUi.actionShowDocument, settings);
    app::saveState("newsgroup", mUi.actionShowOther, settings);
    app::saveState("newsgroup", mUi.actionShowBroken, settings);
    app::saveState("newsgroup", mUi.actionShowDeleted, settings);
    app::saveState("newsgroup", mUi.chkRefreshAutomatically, settings);

    const auto& settingsPath = app::joinPath(mGroupPath, mGroupName);
    const auto& settingsFile = app::joinPath(settingsPath, mGroupName + ".json");

    const auto err = settings.save(settingsFile);
    if (err != QFile::NoError)
    {
        ERROR("Failed to save settings %1, %2", settingsFile, err);
    }
}

MainWidget::info NewsGroup::getInformation() const
{
    return {"group.html", false};
}

Finder* NewsGroup::getFinder()
{
    return this;
}

void NewsGroup::saveState(app::Settings& s)
{
    // this is called if the widget is still alive when the application
    // is exiting. we can save our initial (constructor) args
    // that provide us the information needed to re-load the actual group data.
    s.set("newsgroup", "account", mAccountId);
    s.set("newsgroup", "group_path", mGroupPath);
    s.set("newsgroup", "group_name", mGroupName);

    saveState();
}

void NewsGroup::loadState(app::Settings& s)
{
    // this is called if the widget was alive when the application
    // was exited. then we persisted our widget ctor info into settings
    // and we can load that now.
    // that info will tell us where to look for the actual data files.
    mAccountId = s.get("newsgroup", "account").toUInt();
    mGroupPath = s.get("newsgroup", "group_path").toString();
    mGroupName = s.get("newsgroup", "group_name").toString();

    mUi.grpView->setTitle(app::toString("%1 (%2 / %3)", mGroupName, app::count {0}, app::count{0}));
    setWindowTitle(mGroupName);

    loadState();
    loadFirstData();
}

bool NewsGroup::isMatch(const QString& str, std::size_t index, bool caseSensitive)
{
    const auto& article = mModel.getArticle(index);
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
    const auto& article = mModel.getArticle(index);
    const auto& subject = article.subject();
    const auto& latin   = QString::fromLatin1(subject.c_str(), subject.size());
    if (reg.indexIn(latin) != -1)
        return true;
    return false;
}

std::size_t NewsGroup::numItems() const
{
    return mModel.numShown();
}

std::size_t NewsGroup::curItem() const
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return 0;

    const auto& first = indices.first();
    return first.row();
}

void NewsGroup::setFound(std::size_t index)
{
    auto* model = mUi.tableView->model();
    auto i = model->index(index, 0);
    mUi.tableView->setCurrentIndex(i);
    mUi.tableView->scrollTo(i);
}

void NewsGroup::loadFirstData()
{
    try
    {
        if (!mModel.init(mGroupPath, mGroupName))
        {
            mModel.refresh(mAccountId, mGroupPath, mGroupName);
            mUi.progressBar->setVisible(true);
            mUi.progressBar->setMaximum(0),
            mUi.progressBar->setMinimum(0);
            mUi.actionStop->setEnabled(true);
            mUi.actionRefresh->setEnabled(false);
            mUi.btnLoadMore->setVisible(false);
        }
        else
        {
            mModel.load(0);
            const bool refresh_automatically = mUi.chkRefreshAutomatically->isChecked();
            if (refresh_automatically) {
                on_actionRefresh_triggered();
            }
        }
    }
    catch (const std::exception& e)
    {
        mUi.progressBar->setVisible(false);
        mUi.loader->setVisible(false);
        QMessageBox::critical(this, mGroupName,
            tr("Unable to load the newsgroup data.\n%1").arg(app::widen(e.what())));
    }
}

void NewsGroup::deleteData(quint32 account, QString path, QString group)
{
    app::NewsGroup::deleteData(account, std::move(path), std::move(group));
}

void NewsGroup::on_actionShowNone_changed()
{
    mModel.setTypeFilter(FileType::none, mUi.actionShowNone->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionShowAudio_changed()
{
    mModel.setTypeFilter(FileType::audio, mUi.actionShowAudio->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionShowVideo_changed()
{
    mModel.setTypeFilter(FileType::video, mUi.actionShowVideo->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionShowImage_changed()
{
    mModel.setTypeFilter(FileType::image, mUi.actionShowImage->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionShowText_changed()
{
    mModel.setTypeFilter(FileType::text, mUi.actionShowText->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionShowArchive_changed()
{
    mModel.setTypeFilter(FileType::archive, mUi.actionShowArchive->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionShowParity_changed()
{
    mModel.setTypeFilter(FileType::parity, mUi.actionShowParity->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionShowDocument_changed()
{
    mModel.setTypeFilter(FileType::document, mUi.actionShowDocument->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionShowOther_changed()
{
    mModel.setTypeFilter(FileType::other, mUi.actionShowOther->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionShowBroken_changed()
{
    mModel.setFlagFilter(FileFlag::broken, mUi.actionShowBroken->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionShowDeleted_changed()
{
    mModel.setFlagFilter(FileFlag::deleted, mUi.actionShowDeleted->isChecked());
    mModel.applyFilter();
}

void NewsGroup::on_actionDelete_triggered()
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    mModel.killSelected(indices);
}

void NewsGroup::on_actionHeaders_triggered()
{
    DlgVolumes dlg(this, mModel);
    dlg.exec();
}

void NewsGroup::on_actionRefresh_triggered()
{
    mModel.refresh(mAccountId, mGroupPath, mGroupName);
    mUi.actionRefresh->setEnabled(false);
    mUi.actionStop->setEnabled(true);
    mUi.progressBar->setMaximum(0);
    mUi.progressBar->setMinimum(0);
    mUi.progressBar->setVisible(true);
}

void NewsGroup::on_actionFilter_triggered()
{
    DlgFilter::Params filter = mFilterParams;

    DlgFilter dlg(this, filter);
    dlg.applyFilter = [this](quint32 minDays, quint32 maxDays, quint64 minSize, quint64 maxSize,
        const QString& matchString, bool matchStringCaseSensitive) {
        mModel.setSizeFilter(minSize, maxSize);
        mModel.setDateFilter(minDays, maxDays);
        mModel.setStringFilter(matchString, matchStringCaseSensitive);
        mModel.applyFilter();
    };
    if (dlg.exec() == QDialog::Accepted)
    {
        mFilterParams = filter;
        if (dlg.isApplied())
            return;
    }

    mModel.setSizeFilter(mFilterParams.minSize, mFilterParams.maxSize);
    mModel.setDateFilter(mFilterParams.minDays, mFilterParams.maxDays);
    mModel.setStringFilter(mFilterParams.matchString, mFilterParams.matchStringCaseSensitive);
    mModel.applyFilter();
}

void NewsGroup::on_actionStop_triggered()
{
    mModel.stop();
    mUi.actionStop->setEnabled(false);
    mUi.progressBar->setVisible(false);
    mUi.actionRefresh->setEnabled(true);
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
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    mModel.bookmark(indices);
}

void NewsGroup::on_actionBookmarkPrev_triggered()
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    int start = indices.isEmpty() ? 0
        : indices[0].row() - 1;

    const auto numItems = mModel.numShown();
    for (std::size_t i=0; i<numItems; ++i, --start)
    {
        if (start < 0)
            start = numItems - 1;

        const auto& item = mModel.getArticle(start);
        if ( item.is_bookmarked())
        {
            setFound(start);
            break;
        }
    }
}

void NewsGroup::on_actionBookmarkNext_triggered()
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    const auto start = indices.isEmpty() ? 0
        : indices[0].row() + 1;

    const auto numItems = mModel.numShown();
    for (std::size_t i=0; i<numItems; ++i)
    {
        const auto index = (start + i) % numItems;
        const auto& item = mModel.getArticle(index);
        if (item.is_bookmarked())
        {
            setFound(index);
            break;
        }
    }
}

void NewsGroup::on_btnLoadMore_clicked()
{
    mUi.loader->setVisible(true);
    mUi.btnLoadMore->setEnabled(false);

    mModel.load();

    mUi.loader->setVisible(false);

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
    sub.addAction(mUi.actionBrowse);
    sub.setEnabled(mUi.actionDownload->isEnabled());

    QMenu menu(this);
    menu.addAction(mUi.actionRefresh);
    menu.addSeparator();
    menu.addAction(mUi.actionHeaders);
    menu.addSeparator();
    menu.addAction(mUi.actionDownload);
    menu.addMenu(&sub);
    menu.addSeparator();
    menu.addAction(mUi.actionFilter);


    QMenu showType("Filter by type", this);
    showType.setIcon(QIcon("icons:ico_filter.png"));
    showType.addAction(mUi.actionShowNone);
    showType.addAction(mUi.actionShowAudio);
    showType.addAction(mUi.actionShowVideo);
    showType.addAction(mUi.actionShowImage);
    showType.addAction(mUi.actionShowText);
    showType.addAction(mUi.actionShowArchive);
    showType.addAction(mUi.actionShowParity);
    showType.addAction(mUi.actionShowDocument);
    showType.addAction(mUi.actionShowOther);

    QMenu showFlag("Filter by status", this);
    showFlag.setIcon(QIcon("icons:ico_filter.png"));
    showFlag.addAction(mUi.actionShowBroken);
    showFlag.addAction(mUi.actionShowDeleted);

    menu.addMenu(&showType);
    menu.addMenu(&showFlag);
    menu.addSeparator();
    menu.addAction(mUi.actionBookmarkPrev);
    menu.addAction(mUi.actionBookmark);
    menu.addAction(mUi.actionBookmarkNext);
    menu.addSeparator();
    menu.addAction(mUi.actionDelete);
    menu.addSeparator();
    menu.addAction(mUi.actionStop);
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
    mModel.select(mCurrentlySelectedRows, false);

    auto* selectionModel = mUi.tableView->selectionModel();

    // grab new items.
    mCurrentlySelectedRows = selectionModel->selectedRows();
    mCurrentIndex = selectionModel->currentIndex();

    // update (store) the item bits.
    mModel.select(mCurrentlySelectedRows, true);

    const auto empty = mCurrentlySelectedRows.empty();

    mUi.actionDownload->setEnabled(!empty);
    mUi.actionBookmark->setEnabled(!empty);
    mUi.actionDelete->setEnabled(!empty);
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
    mModel.scanSelected(list);

    auto* selectionModel = mUi.tableView->selectionModel();
    selectionModel->blockSignals(true);

    if (!list.empty())
    {
        QItemSelection sel;
        for (const auto& i : list)
            sel.select(i, i);

        selectionModel->select(sel,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    selectionModel->setCurrentIndex(mCurrentIndex,
        QItemSelectionModel::NoUpdate);
    selectionModel->blockSignals(false);

    mCurrentlySelectedRows = list;

    const auto numShown = mModel.numShown();
    const auto numItems = mModel.numItems();
    mUi.grpView->setTitle(app::toString("%1 (%2 / %3)", mGroupName, app::count{numShown}, app::count{numItems}));
}

void NewsGroup::modelLayoutChanged()
{
    // just forward this.
    modelEndReset();
}

void NewsGroup::newHeaderInfoAvailable(const app::HeaderUpdateInfo& info)
{
    const QString& file = info.groupFile;
    const QString& name = info.groupName;

    // it's possible that the local first is actually
    // smaller than the remote first. this can happen when the
    // articles become too old and the server doesn't keep them around.
    // then when this happens is that our local number of articles
    // will actually exceed the number of articles on the remote
    // and this will throw off the progress computation.
    // if this happens we clip the local article range
    // and consider only the newest articles in the progress computation.
    // see issue #103
    const auto local_first  = std::max(info.localFirst, info.remoteFirst);
    const auto local_last   = info.localLast;
    const auto remote_first = info.remoteFirst;
    const auto remote_last  = info.remoteLast;
    const auto num_local_articles = local_last - local_first + 1;
    const auto num_remote_articles = remote_last - remote_first + 1;

    if (file.indexOf(app::joinPath(mGroupPath, mGroupName)) != 0)
        return;

    const auto max = std::numeric_limits<int>::max();
    if(num_local_articles < max && num_remote_articles < max)
    {
        mUi.progressBar->setMaximum(num_remote_articles);
        mUi.progressBar->setValue(num_local_articles);
    }
    else
    {
        const auto d = double(num_remote_articles) / double(num_local_articles);
        mUi.progressBar->setMaximum(max);
        mUi.progressBar->setValue(d * max);
    }

    const auto numAvail  = mModel.numBlocksAvail();
    const auto numLoaded = mModel.numBlocksLoaded();
    mUi.btnLoadMore->setEnabled(numAvail != numLoaded);
    mUi.btnLoadMore->setVisible(true);
    mUi.btnLoadMore->setText(tr("Load more ... (%1/%2)").arg(numLoaded).arg(numAvail));
}

void NewsGroup::updateCompleted(const app::HeaderInfo& info)
{
    if (info.groupName != mGroupName ||
        info.groupPath != mGroupPath) return;

    mUi.progressBar->setVisible(false);
    mUi.actionStop->setEnabled(false);
    mUi.actionRefresh->setEnabled(true);
}


void NewsGroup::downloadSelected(const QString& folder)
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    Q_ASSERT(mAccountId);

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
    const auto media    = mModel.findMediaType();
    const auto byteSize = mModel.sumDataSizes(indices);
    const auto desc = mModel.suggestName(indices);
    if (!passSpaceCheck(this, desc, folder, byteSize, byteSize, media))
        return;

    mModel.download(indices, mAccountId, folder);
}

bool NewsGroup::eventFilter(QObject* recv, QEvent* event)
{
    Q_ASSERT(recv == mUi.tableView);

    if (event->type() == QEvent::KeyPress)
    {
        const auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Space)
        {
            on_btnLoadMore_clicked();
        }
        else if (key->key() == Qt::Key_Escape)
        {
            mUi.tableView->clearSelection();
        }
    }
    return false;
}

} // gui
