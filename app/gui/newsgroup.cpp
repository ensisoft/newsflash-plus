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

#define LOGTAG "news"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMessageBox>
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#  include <QtGui/QPixmap>
#  include <QFile>
#  include <QFileInfo>
#  include <QDir>
#include <newsflash/warnpop.h>
#include <limits>
#include "newsgroup.h"
#include "mainwindow.h"
#include "dlgfilter.h"
#include "dlgvolumes.h"
#include "../debug.h"
#include "../format.h"
#include "../utility.h"
#include "../engine.h"
#include "../fileinfo.h"
#include "../types.h"

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
    QObject::connect(app::g_engine, SIGNAL(newHeaderDataAvailable(const QString&)),
        this, SLOT(newHeaderDataAvailable(const QString&)));
    QObject::connect(app::g_engine, SIGNAL(updateCompleted(const app::HeaderInfo&)),
        this, SLOT(updateCompleted(const app::HeaderInfo&)));    

    QObject::connect(&model_, SIGNAL(modelAboutToBeReset()), 
        this, SLOT(modelBegReset()));
    QObject::connect(&model_, SIGNAL(modelReset()), 
        this, SLOT(modelEndReset()));

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

    filter_.minSize  = 0;
    filter_.maxSize  = std::numeric_limits<quint32>::max() + 1ull;
    filter_.bMinSize = false;
    filter_.bMaxSize = false;
    filter_.minDays  = 0;
    filter_.maxDays  = 12527;
    filter_.bMinDays = false;
    filter_.bMaxDays = false;
    filter_.sizeUnits = DlgFilter::Unit::MB;
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
    bar.addAction(ui_.actionDelete);
    bar.addSeparator();
    bar.addAction(ui_.actionStop);
}

void NewsGroup::addActions(QMenu& menu)
{

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
    dlg.applyFilter = [this](quint32 minDays, quint32 maxDays, quint64 minSize, quint64 maxSize) {
        model_.setSizeFilter(minSize, maxSize);
        model_.setDateFilter(minDays, maxDays);
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

void NewsGroup::on_btnLoadMore_clicked()
{
    ui_.loader->setVisible(true);
    ui_.btnLoadMore->setEnabled(false);

    model_.load();
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

void NewsGroup::selectionChanged(const QItemSelection& next, const QItemSelection& prev)
{
    // store the currently selected list of items
    // so we can restore it later.
    model_.select(next.indexes(), true);
    model_.select(prev.indexes(), false);

    const auto empty = next.indexes().empty();
    ui_.actionDownload->setEnabled(!empty);
}

void NewsGroup::modelBegReset()
{}

void NewsGroup::modelEndReset()
{
    QModelIndexList list;
    model_.scanSelected(list);

    if (!list.empty())
    {
        QItemSelection sel;
        for (const auto& i : list)
            sel.select(i, i);

        auto* model = ui_.tableView->selectionModel();
        model->select(sel, 
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    const auto numShown = model_.numShown();
    const auto numItems = model_.numItems();
    ui_.grpView->setTitle(app::toString("%1 (%2 / %3)", name_, app::count{numShown}, app::count{numItems}));
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

void NewsGroup::newHeaderDataAvailable(const QString& file)
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


void NewsGroup::downloadSelected(QString folder)
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    Q_ASSERT(account_);

    model_.download(indices, account_, folder);
}

} // gui
