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
#include "../debug.h"
#include "../format.h"
#include "../utility.h"
#include "../engine.h"
#include "../fileinfo.h"

namespace gui
{

NewsGroup::NewsGroup(quint32 acc, QString path, QString name) : account_(acc), path_(path), name_(name)
{
    using Cols = app::NewsGroup::Columns;

    ui_.setupUi(this);
    ui_.tableView->setModel(&model_);
    ui_.grpView->setTitle(name + " (0)");
    ui_.tableView->setColumnWidth((int)Cols::DownloadFlag, 32);
    ui_.tableView->setColumnWidth((int)Cols::BrokenFlag, 32);
    ui_.tableView->setColumnWidth((int)Cols::BookmarkFlag, 32);
    ui_.progressBar->setVisible(false);
    ui_.loader->setVisible(false);
    ui_.actionStop->setEnabled(false);

    QObject::connect(app::g_engine, SIGNAL(newHeaderInfoAvailable(const QString&, quint64, quint64)),
        this, SLOT(newHeaderInfoAvailable(const QString&, quint64, quint64)));
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

    model_.onLoadProgress = [this] (std::size_t curItem, std::size_t numItems) {
        ui_.loader->setVisible(true);
        ui_.loader->setMaximum(numItems);
        ui_.loader->setValue(curItem);
        if ((curItem % 150) == 0)
            QCoreApplication::processEvents();
    };
    loadingState_ = false;
}

NewsGroup::~NewsGroup()
{}

void NewsGroup::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionRefresh);
    bar.addSeparator();
    bar.addAction(ui_.actionFilter);
    bar.addSeparator();
    bar.addAction(ui_.actionStop);
}

void NewsGroup::addActions(QMenu& menu)
{

}

void NewsGroup::loadState(app::Settings& settings)
{
    DEBUG("Enter loadState");

    ui_.tableView->setSortingEnabled(false);

    app::loadTableLayout("newsgroup", ui_.tableView, settings);

    ui_.tableView->setSortingEnabled(true);
    ui_.tableView->sortByColumn((int)app::NewsGroup::Columns::Age, 
        Qt::DescendingOrder);

    DEBUG("Leave loadState");
}

void NewsGroup::saveState(app::Settings& settings)
{
    app::saveTableLayout("newsgroup", ui_.tableView, settings);
}

MainWidget::info NewsGroup::getInformation() const 
{
    return {"group.html", false};
}

bool NewsGroup::canClose() const
{
    // this is a bit of a hack here because
    // we have the processEvents() in the loading loop
    // it means that its possible for the MainWindow to want to
    // react to user input and close the widget.
    // which deletes us, which dumps core.
    // so simply, if we're loading data we cannot be closed.
    if (loadingState_)
        return false;

    return true;
}

void NewsGroup::load()
{
    blockIndex_ = 0;

    ui_.btnLoadMore->setEnabled(false);

    quint32 numBlocks = 0;

    loadingState_ = true;

    if (!model_.load(blockIndex_, path_, name_, numBlocks))
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
        blockIndex_ = 1;
        ui_.loader->setVisible(false);
        if (numBlocks > 1) {
            ui_.btnLoadMore->setEnabled(true);
            ui_.btnLoadMore->setText(tr("Load older headers ... (%1/%2)").arg(blockIndex_).arg(numBlocks));
        }
    }

    loadingState_ = false;
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
{}

void NewsGroup::on_actionStop_triggered()
{
    model_.stop();
    ui_.actionStop->setEnabled(false);
    ui_.progressBar->setVisible(false);
    ui_.actionRefresh->setEnabled(true);
}

void NewsGroup::on_btnLoadMore_clicked()
{
    loadingState_ = true;
    ui_.btnLoadMore->setEnabled(false);

    quint32 numBlocks = 0;
    model_.load(blockIndex_, path_, name_, numBlocks);

    ++blockIndex_;
    ui_.loader->setVisible(false);
    ui_.btnLoadMore->setEnabled(true);
    ui_.btnLoadMore->setText(tr("Load older headers ... (%1/%2)").arg(blockIndex_).arg(numBlocks));
    ui_.btnLoadMore->setEnabled(blockIndex_ != numBlocks);
    loadingState_ = false;
}

void NewsGroup::on_tableView_customContextMenuRequested(QPoint p)
{
    QMenu menu(this);
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();
    menu.addAction(ui_.actionFilter);
    menu.addSeparator();
    menu.addAction(ui_.actionStop);
    menu.exec(QCursor::pos());
}

void NewsGroup::selectionChanged(const QItemSelection& next, const QItemSelection& prev)
{
    // store the currently selected list of items
    // so we can restore it later.
    model_.select(next.indexes(), true);
    model_.select(prev.indexes(), false);
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
        model->setCurrentIndex(list[0], 
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        model->select(sel, 
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    const auto numItems = model_.numItems();
    ui_.grpView->setTitle(QString("%1 (%2)").arg(name_).arg(numItems));
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

void NewsGroup::updateCompleted(const app::HeaderInfo& info)
{
    if (info.groupName != name_ ||
        info.groupPath != path_) return;

    ui_.progressBar->setVisible(false);
    ui_.actionStop->setEnabled(false);
    ui_.actionRefresh->setEnabled(true);
}


} // gui
