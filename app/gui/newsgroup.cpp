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
    ui_.actionStop->setEnabled(false);

    QObject::connect(app::g_engine, SIGNAL(newHeaderInfoAvailable(const QString&, quint64, quint64)),
        this, SLOT(newHeaderInfoAvailable(const QString&, quint64, quint64)));
    QObject::connect(app::g_engine, SIGNAL(updateCompleted(const app::HeaderInfo&)),
        this, SLOT(updateCompleted(const app::HeaderInfo&)));    

    QObject::connect(&model_, SIGNAL(modelReset()), this, SLOT(modelReset()));

    setWindowTitle(name);
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
    ui_.tableView->setSortingEnabled(false);

    app::loadTableLayout("newsgroup", ui_.tableView, settings);

    ui_.tableView->setSortingEnabled(true);
    ui_.tableView->sortByColumn((int)app::NewsGroup::Columns::Age, 
        Qt::DescendingOrder);
}

void NewsGroup::saveState(app::Settings& settings)
{
    app::saveTableLayout("newsgroup", ui_.tableView, settings);
}

void NewsGroup::load()
{
    blockIndex_ = 0;

    if (!model_.load(blockIndex_, path_, name_))
    {
        model_.refresh(account_, path_, name_);
        ui_.progressBar->setVisible(true);
        ui_.progressBar->setMaximum(0),
        ui_.progressBar->setMinimum(0);
        ui_.actionStop->setEnabled(true);
        ui_.btnLoadMore->setVisible(false);
    } 
    else
    {
        blockIndex_ = 1;
    }
}

void NewsGroup::on_actionRefresh_triggered()
{
    model_.refresh(account_, path_, name_);
    ui_.actionRefresh->setEnabled(false);
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
    if (!model_.load(blockIndex_, path_, name_))
    {
        ui_.btnLoadMore->setEnabled(false);
    }
    ++blockIndex_;
}

void NewsGroup::modelReset()
{
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
