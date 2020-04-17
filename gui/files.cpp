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
#  include <QMenu>
#  include <QToolBar>
#  include <QMessageBox>
#  include <QFileDialog>
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

Files::Files(app::Files& files) : mModel(files)
{
    mUi.setupUi(this);
    mUi.tableFiles->setModel(&mModel);
    mUi.actionOpenFile->setShortcut(QKeySequence::Open);

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
    menu.addAction(mUi.actionOpenFile);
    menu.addSeparator();
    menu.addAction(mUi.actionOpenFolder);
    menu.addSeparator();
    menu.addAction(mUi.actionClear);
    menu.addSeparator();
    menu.addAction(mUi.actionDelete);
    menu.addAction(mUi.actionForget);

}

void Files::addActions(QToolBar& bar)
{
    bar.addAction(mUi.actionOpenFile);
    bar.addSeparator();
    bar.addAction(mUi.actionOpenFolder);
    bar.addSeparator();
    bar.addAction(mUi.actionClear);
    bar.addSeparator();
    bar.addAction(mUi.actionDelete);
    bar.addAction(mUi.actionForget);
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

    mUi.chkKeepSorted->setChecked(sorted);
    mUi.chkClearOnExit->setChecked(clear);
    mUi.chkConfirmDelete->setChecked(confirm);
    mUi.tableFiles->sortByColumn(sortColumn, (Qt::SortOrder)sortOrder);

    app::loadTableLayout("files", mUi.tableFiles, settings);

    mModel.loadHistory();
    mModel.keepSorted(sorted);
    mNumFiles = mModel.numFiles();
}

void Files::saveState(app::Settings& settings)
{
    const auto sorted    = mUi.chkKeepSorted->isChecked();
    const auto clear     = mUi.chkClearOnExit->isChecked();
    const auto confirm   = mUi.chkConfirmDelete->isChecked();

    const QHeaderView* header = mUi.tableFiles->horizontalHeader();
    const auto sortColumn = header->sortIndicatorSection();
    const auto sortOrder  = header->sortIndicatorOrder();

    settings.set("files", "keep_sorted", sorted);
    settings.set("files", "clear_on_exit", clear);
    settings.set("files", "confirm_file_delete", confirm);
    settings.set("files", "sort_column", sortColumn);
    settings.set("files", "sort_order", sortOrder);

    app::saveTableLayout("files", mUi.tableFiles, settings);
}

void Files::shutdown()
{
    const auto clear = mUi.chkClearOnExit->isChecked();
    if (clear)
    {
        mModel.eraseHistory();
    }
}

void Files::refresh(bool isActive)
{
    if (isActive)
        return;

    const auto numFiles = mModel.numFiles();
    if (numFiles > mNumFiles)
        setWindowTitle(QString("Files (%1)").arg(numFiles - mNumFiles));
}

void Files::activate(QWidget*)
{
    mNumFiles = mModel.numFiles();
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
    const auto& item = mModel.getItem(index);
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
    const auto& item = mModel.getItem(index);
    if (regex.indexIn(item.name) != -1)
        return true;
    return false;
}

std::size_t Files::numItems() const
{ return mModel.numFiles(); }

std::size_t Files::curItem() const
{
    const auto& indices = mUi.tableFiles->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return 0;

    const auto& first = indices.first();
    return first.row();
}

void Files::setFound(std::size_t index)
{
    auto* model = mUi.tableFiles->model();
    auto i = model->index(index, 0);
    mUi.tableFiles->setCurrentIndex(i);
    mUi.tableFiles->scrollTo(i);
}

void Files::on_actionOpenFile_triggered()
{
    const auto& indices = mUi.tableFiles->selectionModel()->selectedRows();
    for (int i=0; i<indices.size(); ++i)
    {
        const auto row   = indices[i].row();
        const auto& item = mModel.getItem(row);
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

    const auto& indices = mUi.tableFiles->selectionModel()->selectedRows();
    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        const auto& item = mModel.getItem(row);
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
    mModel.eraseHistory();
}

void Files::on_actionOpenFolder_triggered()
{
    const auto& indices = mUi.tableFiles->selectionModel()->selectedRows();
    for (int i=0; i<indices.size(); ++i)
    {
        const auto row   = indices[i].row();
        const auto& item = mModel.getItem(row);
        app::openFolder(item.path);
    }
}

void Files::on_actionDelete_triggered()
{
    auto indices = mUi.tableFiles->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto confirm = mUi.chkConfirmDelete->isChecked();
    if (confirm)
    {
        DlgConfirm dlg(this);
        if (dlg.exec() == QDialog::Rejected)
            return;
        mUi.chkConfirmDelete->setChecked(!dlg.askAgain());
    }

    mModel.deleteFiles(indices);
}

void Files::on_actionForget_triggered()
{
    auto indices = mUi.tableFiles->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    mModel.forgetFiles(indices);
}


void Files::on_tableFiles_customContextMenuRequested(QPoint point)
{
    newsflash::bitflag<app::FileType> types;

    const auto& indices = mUi.tableFiles->selectionModel()->selectedRows();
    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        const auto& item = mModel.getItem(row);
        const auto type = app::findFileType(item.name);
        types.set(type);
    }

    const auto& all_tools = app::g_tools->get_tools();
    const auto& sum_tools = app::g_tools->get_tools(types);

    QMenu menu(this);
    menu.addAction(mUi.actionOpenFile);
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
    sub.addAction(mUi.actionOpenFileWith);
    menu.addMenu(&sub);
    menu.addSeparator();
    menu.addAction(mUi.actionOpenFolder);
    menu.addSeparator();
    menu.addAction(mUi.actionDelete);
    menu.addAction(mUi.actionForget);
    menu.exec(QCursor::pos());
}

void Files::on_tableFiles_doubleClicked()
{
    on_actionOpenFile_triggered();
}

void Files::on_chkKeepSorted_clicked()
{
    mModel.keepSorted(mUi.chkKeepSorted->isChecked());
}

void Files::invokeTool()
{
    const auto& indices = mUi.tableFiles->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto* action = qobject_cast<QAction*>(sender());
    const auto  guid = action->property("tool-guid").toULongLong();
    const auto* tool = app::g_tools->get_tool(guid);
    Q_ASSERT(tool);

    for (int i=0; i<indices.size(); ++i)
    {
        const auto row   = indices[i].row();
        const auto& item = mModel.getItem(row);
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
