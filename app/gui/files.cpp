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

#define LOGTAG "files"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QMessageBox>
#include <newsflash/warnpop.h>

#include "files.h"
#include "../debug.h"
#include "../format.h"
#include "../settings.h"
#include "../platform.h"

namespace gui
{

files::files()
{
    ui_.setupUi(this);
    ui_.tableFiles->setModel(&model_);

    DEBUG("files gui created");
}

files::~files()
{
    DEBUG("files gui deleted");
}

void files::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionOpenFile);
    menu.addSeparator();
    menu.addAction(ui_.actionOpenFolder);
    menu.addSeparator();
    menu.addAction(ui_.actionFind);
    menu.addAction(ui_.actionFindPrev);
    menu.addAction(ui_.actionFindNext);
    menu.addSeparator();
    menu.addAction(ui_.actionClear);
    menu.addSeparator();
    menu.addAction(ui_.actionDelete);

}

void files::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionOpenFile);
    bar.addSeparator();
    bar.addAction(ui_.actionOpenFolder);
    bar.addSeparator();
    bar.addAction(ui_.actionFind);
    bar.addAction(ui_.actionFindPrev);
    bar.addAction(ui_.actionFindNext);
    bar.addSeparator();
    bar.addAction(ui_.actionClear);
    bar.addSeparator();
    bar.addAction(ui_.actionDelete);
}

void files::loadstate(app::settings& s) 
{
    const auto sorted  = s.get("files", "keep_sorted", false);
    const auto clear   = s.get("files", "clear_on_exit", false);
    const auto confirm = s.get("files", "confirm_file_delete", true);

    ui_.chkKeepSorted->setChecked(sorted);
    ui_.chkClearOnExit->setChecked(clear);
    ui_.chkConfirmDelete->setChecked(confirm);

    const auto* model = ui_.tableFiles->model();
    for (int i=0; i<model->columnCount()-1; ++i)
    {
        const auto name  = QString("table_col_%1_width").arg(i);
        const auto width = s.get("files", name, ui_.tableFiles->columnWidth(i));
        ui_.tableFiles->setColumnWidth(i, width);
    }

    model_.loadHistory();
}

bool files::savestate(app::settings& s)
{
    const auto sorted  = ui_.chkKeepSorted->isChecked();
    const auto clear   = ui_.chkClearOnExit->isChecked();
    const auto confirm = ui_.chkConfirmDelete->isChecked();

    s.set("files", "keep_sorted", sorted);
    s.set("files", "clear_on_exit", clear);
    s.set("files", "confirm_file_delete", confirm);

    const auto* model = ui_.tableFiles->model();
    // the last column has auto-stretch flag set so it's width
    // is implied. and in fact setting the width will cause rendering bugs
    for (int i=0; i<model->columnCount()-1; ++i)
    {
        const auto width = ui_.tableFiles->columnWidth(i);
        const auto name  = QString("table_col_%1_width").arg(i);
        s.set("files", name, width);
    }    

    return true;
}

void files::shutdown() 
{
    const auto clear = ui_.chkClearOnExit->isChecked();
    if (clear)
    {
        model_.eraseHistory();
    }
}

void files::on_actionOpenFile_triggered()
{
    const auto& indices = ui_.tableFiles->selectionModel()->selectedRows();
    for (int i=0; i<indices.size(); ++i)
    {
        const auto row   = indices[i].row();
        const auto& item = model_.getItem(row);
        const auto& file = QString("%1/%2").arg(item.path).arg(item.name);
        app::open_file(file);
    }
}

void files::on_actionOpenFileWith_triggered()
{

}

void files::on_actionOpenFolder_triggered()
{

}

void files::on_actionClear_triggered()
{
    model_.eraseHistory();
}

void files::on_tableFiles_customContextMenuRequested(QPoint point)
{
    
}

} // gui