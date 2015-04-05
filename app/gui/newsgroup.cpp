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
#include "newsgroup.h"
#include "../debug.h"
#include "../format.h"
#include "../utility.h"

namespace gui
{

NewsGroup::NewsGroup(quint32 acc, QString path, QString name) 
    : account_(acc), path_(path), name_(name)
{
    //using Cols = app::NewsGroup::Columns;

    ui_.setupUi(this);
    ui_.tableView->setModel(&model_);
    ui_.grpView->setTitle(name);
    // ui_.tableView->setColumnWidth((int)Cols::BinaryFlag, 32);
    // ui_.tableView->setColumnWidth((int)Cols::NewFlag, 32);
    // ui_.tableView->setColumnWidth((int)Cols::DownloadFlag, 32);
    // ui_.tableView->setColumnWidth((int)Cols::BrokenFlag, 32);
    // ui_.tableView->setColumnWidth((int)Cols::BookmarkFlag, 32);

    ui_.progressBar->setVisible(false);
}

NewsGroup::~NewsGroup()
{}

void NewsGroup::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionRefresh);
    bar.addSeparator();
    bar.addAction(ui_.actionFilter);
}

void NewsGroup::addActions(QMenu& menu)
{

}

void NewsGroup::loadState(app::Settings& settings)
{
    app::loadTableLayout("newsgroup", ui_.tableView, settings);
}

void NewsGroup::saveState(app::Settings& settings)
{
    app::saveTableLayout("newsgroup", ui_.tableView, settings);
}

void NewsGroup::load()
{
    if (!model_.load(account_, path_, name_))
    {
        ui_.progressBar->setVisible(true);
        ui_.progressBar->setMaximum(0),
        ui_.progressBar->setMinimum(0);
    }
}

void NewsGroup::on_actionRefresh_triggered()
{

}

void NewsGroup::on_actionFilter_triggered()
{}


} // gui
