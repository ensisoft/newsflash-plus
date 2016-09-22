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

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QMessageBox>
#  include <QtGui/QFileDialog>
#  include <QDir>
#  include <QProcess>
#  include <QFileInfo>
#  include <QRegExp>
#include "newsflash/warnpop.h"

#include "mainwindow.h"
#include "files.h"
#include "dlgconfirm.h"
#include "app/debug.h"
#include "app/format.h"
#include "app/settings.h"
#include "app/platform.h"
#include "app/tools.h"
#include "app/eventlog.h"
#include "app/files.h"
#include "app/utility.h"

namespace gui
{

Files::Files(app::Files& files) : model_(files), numFiles_(0)
{
    ui_.setupUi(this);
    ui_.tableFiles->setModel(&model_);
    ui_.actionOpenFile->setShortcut(QKeySequence::Open);

    QObject::connect(app::g_tools, SIGNAL(toolsUpdated()),
        this, SLOT(toolsUpdated()));

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

void Files::loadState(app::Settings& settings) 
{
    const auto sorted     = settings.get("files", "keep_sorted", false);
    const auto clear      = settings.get("files", "clear_on_exit", false);
    const auto confirm    = settings.get("files", "confirm_file_delete", true);
    const auto sortColumn = settings.get("files", "sort_column", 0);
    const auto sortOrder  = settings.get("files", "sort_order", (int)Qt::AscendingOrder);

    ui_.chkKeepSorted->setChecked(sorted);
    ui_.chkClearOnExit->setChecked(clear);
    ui_.chkConfirmDelete->setChecked(confirm);
    ui_.tableFiles->sortByColumn(sortColumn, (Qt::SortOrder)sortOrder);    

    app::loadTableLayout("files", ui_.tableFiles, settings);

    model_.loadHistory();
    model_.keepSorted(sorted);
    numFiles_ = model_.numFiles();
}

void Files::saveState(app::Settings& settings)
{
    const auto sorted    = ui_.chkKeepSorted->isChecked();
    const auto clear     = ui_.chkClearOnExit->isChecked();
    const auto confirm   = ui_.chkConfirmDelete->isChecked();

    const QHeaderView* header = ui_.tableFiles->horizontalHeader();
    const auto sortColumn = header->sortIndicatorSection();
    const auto sortOrder  = header->sortIndicatorOrder();

    settings.set("files", "keep_sorted", sorted);
    settings.set("files", "clear_on_exit", clear);
    settings.set("files", "confirm_file_delete", confirm);
    settings.set("files", "sort_column", sortColumn);
    settings.set("files", "sort_order", sortOrder);

    app::saveTableLayout("files", ui_.tableFiles, settings);
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

MainWidget::info Files::getInformation() const
{
    return {"files.html", true};    
}

Finder* Files::getFinder() 
{
    return this;
}

bool Files::isMatch(const QString& str, std::size_t index, bool caseSensitive)
{
    const auto& item = model_.getItem(index);
    if (!caseSensitive)
    {
        auto upper = item.name.toUpper();
        if (upper.indexOf(str) != -1)
            return true;
    }
    else
    {
        if (item.name.indexOf(str) != -1)
            return true;
    }
    return false;
}

bool Files::isMatch(const QRegExp& regex, std::size_t index)
{
    const auto& item = model_.getItem(index);
    if (regex.indexIn(item.name) != -1)
        return true;
    return false;
}

std::size_t Files::numItems() const 
{ return model_.numFiles(); }

std::size_t Files::curItem() const 
{
    const auto& indices = ui_.tableFiles->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return 0;

    const auto& first = indices.first();
    return first.row();
}

void Files::setFound(std::size_t index)
{
    auto* model = ui_.tableFiles->model();
    auto i = model->index(index, 0);
    ui_.tableFiles->setCurrentIndex(i);
    ui_.tableFiles->scrollTo(i);
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
        app::openFolder(item.path);
    }
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
    menu.addAction(ui_.actionOpenFolder);
    menu.addSeparator();
    menu.addAction(ui_.actionDelete);
    menu.exec(QCursor::pos());
}

void Files::on_tableFiles_doubleClicked()
{
    on_actionOpenFile_triggered();
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

} // gui
