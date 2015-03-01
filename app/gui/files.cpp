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

#define LOGTAG "files"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QMessageBox>
#  include <QtGui/QFileDialog>
#  include <QDir>
#  include <QProcess>
#  include <QFileInfo>
#  include <QRegExp>
#include <newsflash/warnpop.h>

#include "mainwindow.h"
#include "files.h"
#include "dlgconfirm.h"
#include "../debug.h"
#include "../format.h"
#include "../settings.h"
#include "../platform.h"
#include "../tools.h"
#include "../eventlog.h"
#include "../files.h"

namespace gui
{

Files::Files(app::Files& files) : model_(files), numFiles_(0)
{
    ui_.setupUi(this);
    ui_.tableFiles->setModel(&model_);

    QObject::connect(app::g_tools, SIGNAL(toolsUpdated()),
        this, SLOT(toolsUpdated()));

    ui_.actionOpenFile->setShortcut(QKeySequence::Open);
    ui_.actionFind->setShortcut(QKeySequence::Find);
    ui_.actionFindNext->setShortcut(QKeySequence::FindNext);
    ui_.actionFindPrev->setShortcut(QKeySequence::FindPrevious);
    ui_.findContainer->setVisible(false);

    DEBUG("Files UI created");
}

Files::~Files()
{
    DEBUG("Files UI deleted");
}

void Files::addActions(QMenu& menu)
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

void Files::addActions(QToolBar& bar)
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

    bar.addSeparator();

    const auto& tools = app::g_tools->get_tools();

    for (const auto* tool : tools)
    {
        QAction* action = bar.addAction(tool->icon(),tool->name());
        action->setProperty("tool-guid", tool->guid());

        const QString& shortcut = tool->shortcut();
        if (!shortcut.isEmpty())
        {
            QKeySequence keys(shortcut);
            if (!keys.isEmpty())
                action->setShortcut(keys);
        }
        QObject::connect(action, SIGNAL(triggered()), 
            this, SLOT(invokeTool()));
    }    
}

void Files::loadState(app::Settings& s) 
{
    const auto sorted  = s.get("files", "keep_sorted", false);
    const auto clear   = s.get("files", "clear_on_exit", false);
    const auto confirm = s.get("files", "confirm_file_delete", true);
    const auto matchcase = s.get("files", "match_case", true);

    const auto sortColumn = s.get("files", "sort_column", 0);
    const auto sortOrder  = s.get("files", "sort_order", (int)Qt::AscendingOrder);

    ui_.chkKeepSorted->setChecked(sorted);
    ui_.chkClearOnExit->setChecked(clear);
    ui_.chkConfirmDelete->setChecked(confirm);
    ui_.chkMatchCase->setChecked(matchcase);

    const auto* model = ui_.tableFiles->model();
    for (int i=0; i<model->columnCount()-1; ++i)
    {
        const auto name  = QString("table_col_%1_width").arg(i);
        const auto width = s.get("files", name, ui_.tableFiles->columnWidth(i));
        ui_.tableFiles->setColumnWidth(i, width);
    }

    model_.loadHistory();
    model_.keepSorted(sorted);

    ui_.tableFiles->sortByColumn(sortColumn, (Qt::SortOrder)sortOrder);

    numFiles_ = model_.numFiles();
}

void Files::saveState(app::Settings& s)
{
    const auto sorted    = ui_.chkKeepSorted->isChecked();
    const auto clear     = ui_.chkClearOnExit->isChecked();
    const auto confirm   = ui_.chkConfirmDelete->isChecked();
    const auto matchcase = ui_.chkMatchCase->isChecked();

    const QHeaderView* header = ui_.tableFiles->horizontalHeader();
    const auto sortColumn = header->sortIndicatorSection();
    const auto sortOrder  = header->sortIndicatorOrder();

    s.set("files", "keep_sorted", sorted);
    s.set("files", "clear_on_exit", clear);
    s.set("files", "confirm_file_delete", confirm);
    s.set("files", "match_case", matchcase);
    s.set("files", "sort_column", sortColumn);
    s.set("files", "sort_order", sortOrder);

    const auto* model = ui_.tableFiles->model();
    // the last column has auto-stretch flag set so it's width
    // is implied. and in fact setting the width will cause rendering bugs
    for (int i=0; i<model->columnCount()-1; ++i)
    {
        const auto width = ui_.tableFiles->columnWidth(i);
        const auto name  = QString("table_col_%1_width").arg(i);
        s.set("files", name, width);
    }    
}

void Files::shutdown() 
{
    const auto clear = ui_.chkClearOnExit->isChecked();
    if (clear)
    {
        model_.eraseHistory();
    }
}

void Files::refresh(bool isActive)
{
    if (isActive)
        return;

    const auto numFiles = model_.numFiles();
    if (numFiles > numFiles_)
        setWindowTitle(QString("Files (%1)").arg(numFiles - numFiles_));
}

void Files::activate(QWidget*)
{
    numFiles_ = model_.numFiles();
    setWindowTitle("Files");
}

void Files::on_actionOpenFile_triggered()
{
    const auto& indices = ui_.tableFiles->selectionModel()->selectedRows();
    for (int i=0; i<indices.size(); ++i)
    {
        const auto row   = indices[i].row();
        const auto& item = model_.getItem(row);
        const auto& file = QString("%1/%2").arg(item.path).arg(item.name);
        app::openFile(file);
    }
}

void Files::on_actionOpenFileWith_triggered()
{
    QString filter;
#if defined(WINDOWS_OS)
    filter = "(Executables) *.exe";
#endif

    const auto app = QFileDialog::getOpenFileName(this,
        tr("Select Application"), QString(), filter);
    if (app.isEmpty())
        return;

    const auto exe = QDir::toNativeSeparators(app);

    newsflash::bitflag<app::FileType> types;

    const auto& indices = ui_.tableFiles->selectionModel()->selectedRows();
    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        const auto& item = model_.getItem(row);
        const auto& file = QString("\"%1/%2\"").arg(item.path).arg(item.name);
        const auto type = app::findFileType(item.name);
        types.set(type);

        if (!QProcess::startDetached(QString("\"%1\" %2").arg(exe).arg(file)))
        {
            QFileInfo info(exe);
            const auto name = info.completeBaseName();
            QMessageBox::critical(this, name, 
                tr("The application could not be started.\n%1").arg(exe));
            return;
        }
    }

    if (!app::g_tools->has_tool(exe))
    {
        QMessageBox msg(this);
        msg.setText(tr("Do you want me to remember this application?\r\n%1").arg(exe));
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setIcon(QMessageBox::Question);
        if (msg.exec() == QMessageBox::No)
            return;

        app::tools::tool tool;
        tool.setBinary(exe);
        tool.setName(QFileInfo(exe).baseName());
        tool.setTypes(types);
        app::g_tools->add_tool(tool);
    }
}

void Files::on_actionClear_triggered()
{
    model_.eraseHistory();
}

void Files::on_actionOpenFolder_triggered()
{
    const auto& indices = ui_.tableFiles->selectionModel()->selectedRows();
    for (int i=0; i<indices.size(); ++i)
    {
        const auto row   = indices[i].row();
        const auto& item = model_.getItem(row);
        app::openFile(item.path);
    }
}

void Files::on_actionFind_triggered()
{
    ui_.findContainer->setVisible(true);
    ui_.editFind->setFocus();
}

void Files::on_actionFindNext_triggered()
{
    findNext(true);
}

void Files::on_actionFindPrev_triggered()
{
    findNext(false);
}

void Files::on_actionDelete_triggered()
{
    auto indices = ui_.tableFiles->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto confirm = ui_.chkConfirmDelete->isChecked();
    if (confirm)
    {
        DlgConfirm dlg(this);
        if (dlg.exec() == QDialog::Rejected)
            return;
        ui_.chkConfirmDelete->setChecked(!dlg.askAgain());
    }

    model_.eraseFiles(indices);
}



void Files::on_tableFiles_customContextMenuRequested(QPoint point)
{
    newsflash::bitflag<app::FileType> types;

    const auto& indices = ui_.tableFiles->selectionModel()->selectedRows();
    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        const auto& item = model_.getItem(row);
        const auto type = app::findFileType(item.name);
        types.set(type);
    }

    const auto& all_tools = app::g_tools->get_tools();
    const auto& sum_tools = app::g_tools->get_tools(types);

    QMenu menu(this);
    menu.addAction(ui_.actionOpenFile);
    menu.addSeparator();

    for (const auto* tool : sum_tools)
    {
        QAction* action = menu.addAction(tool->icon(), tool->name());
        action->setProperty("tool-guid", tool->guid());

        QObject::connect(action, SIGNAL(triggered()),
            this, SLOT(invokeTool()));
    }

    menu.addSeparator();

    QMenu sub("Open with", &menu);
    for (const auto* tool : all_tools)
    {
        QAction* action = sub.addAction(tool->icon(), tool->name());
        action->setProperty("tool-guid", tool->guid());

        QObject::connect(action, SIGNAL(triggered()),
            this, SLOT(invokeTool()));
    }

    sub.addSeparator();
    sub.addAction(ui_.actionOpenFileWith);

    menu.addMenu(&sub);
    menu.addSeparator();
    //menu.addAction(ui_.actionFind);
    //menu.addAction(ui_.actionFindPrev);
    //menu.addAction(ui_.actionFindNext);
    //menu.addSeparator();
    menu.addAction(ui_.actionOpenFolder);
    menu.addSeparator();
    //menu.addAction(ui_.actionClear);
    //menu.addSeparator();
    menu.addAction(ui_.actionDelete);
    menu.exec(QCursor::pos());
}

void Files::on_tableFiles_doubleClicked()
{
    on_actionOpenFile_triggered();
}

void Files::on_btnCloseFind_clicked()
{
    ui_.findContainer->setVisible(false);
}

void Files::on_btnFindNext_clicked()
{
    findNext(true);
}

void Files::on_btnFindPrev_clicked()
{
    findNext(false);
}

void Files::on_editFind_returnPressed()
{
    findNext(true);
}

void Files::on_chkKeepSorted_clicked()
{
    model_.keepSorted(ui_.chkKeepSorted->isChecked());
}

void Files::invokeTool()
{
    const auto& indices = ui_.tableFiles->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto* action = qobject_cast<QAction*>(sender());
    const auto  guid = action->property("tool-guid").toULongLong();
    const auto* tool = app::g_tools->get_tool(guid);
    Q_ASSERT(tool);

    for (int i=0; i<indices.size(); ++i)
    {
        const auto row   = indices[i].row();
        const auto& item = model_.getItem(row);
        const auto& file = QString("%1/%2").arg(item.path).arg(item.name);

        if (!tool->startNewInstance(file))
        {
            QMessageBox::critical(this, tool->name(),
                tr("The application could not be started.\n%1").arg(tool->binary()));
        }
    }
}

void Files::toolsUpdated()
{
    emit updateMenu(this);
}

void Files::findNext(bool forward)
{
    QRegExp regex(ui_.editFind->text());
    if (!regex.isValid())
    {
        ui_.label->setText("Incorrectly formed search pattern");
        return;
    }
    ui_.label->clear();

    if (ui_.chkMatchCase->isChecked())
        regex.setCaseSensitivity(Qt::CaseSensitive);
    else regex.setCaseSensitivity(Qt::CaseInsensitive);

    int start = 0;

    const auto& indices = ui_.tableFiles->selectionModel()->selectedRows();
    if (!indices.isEmpty())
        start = indices.last().row();

    int num_rows = model_.rowCount(QModelIndex());

    for (int i=0; i<num_rows; ++i)
    {
        if (forward)
            start = (start + 1) % num_rows;
        else if (--start < 0)
            start = num_rows - 1;

        Q_ASSERT(start >= 0);
        Q_ASSERT(start < num_rows);

        const auto& item = model_.getItem(start);
        if (regex.indexIn(item.name) != -1 ||
            regex.indexIn(item.path) != -1)
        {
            QModelIndex index = ui_.tableFiles->model()->index(start, 0);
            ui_.tableFiles->selectionModel()->setCurrentIndex(index, 
                QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            ui_.tableFiles->scrollTo(index);
            return;
        }
    }    
    ui_.label->setText("No matches");
}


} // gui
