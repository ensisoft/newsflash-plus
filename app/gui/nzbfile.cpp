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
#include <newsflash/warnpop.h>

#include "nzbfile.h"
#include "mainwindow.h"
#include "../debug.h"
#include "../format.h"
#include "../eventlog.h"

namespace gui
{
nzbfile::nzbfile()
{
    ui_.setupUi(this);
    ui_.progressBar->setVisible(false);
    ui_.progressBar->setValue(0);
    ui_.progressBar->setRange(0, 0);
    ui_.tableView->setModel(&model_);

    model_.on_ready = [&]() {
        ui_.progressBar->setVisible(false);
    };
}

nzbfile::~nzbfile()
{}

void nzbfile::add_actions(QMenu& menu)
{
    // The open is in the file menu, so we don't repeat it here    
    //menu.addAction(ui_.actionOpen);
    //menu.addSeparator();
    menu.addAction(ui_.actionDownload);
    menu.addSeparator();
    menu.addAction(ui_.actionClear);
    menu.addSeparator();
    menu.addAction(ui_.actionDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionSettings);
}

void nzbfile::add_actions(QToolBar& bar)
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

void nzbfile::ready()
{
    ui_.progressBar->setVisible(false);
}

void nzbfile::on_actionOpen_triggered()
{
    const auto& file = g_win->select_nzb_file();
    if (file.isEmpty())
        return;

    if (model_.load(file))
    {
        ui_.progressBar->setVisible(true);
        ui_.grpBox->setTitle(file);
    }
}

void nzbfile::on_actionDownload_triggered()
{}

void nzbfile::on_actionClear_triggered()
{
    model_.clear();
    ui_.grpBox->setTitle(tr("NZB File"));
    ui_.actionDownload->setEnabled(false);
    ui_.actionDelete->setEnabled(false);
}

void nzbfile::on_listFiles_customContextMenuRequested(QPoint)
{
    //QMenu sub("Download to");
    //sub.setIcon()

    //QMenu menu(this);
}

} // gui


