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

#define LOGTAG "downloads"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QToolBar>
#  include <QMenu>
#  include <QSplitter>
#  include <QMouseEvent>
#  include <QPixmap>
#  include <QMovie>
#include "newsflash/warnpop.h"

#include "mainwindow.h"
#include "downloads.h"
#include "app/debug.h"
#include "app/format.h"
#include "app/eventlog.h"
#include "app/settings.h"
#include "app/engine.h"
#include "app/platform.h"
#include "app/accounts.h"

namespace gui
{

Downloads::Downloads() : panels_y_pos_(0), numDownloads_(0)
{
    ui_.setupUi(this);
    ui_.tableConns->setModel(&conns_);
    ui_.tableTasks->setModel(&tasks_);
    ui_.splitter->installEventFilter(this);

    const auto defwidth = ui_.tableConns->columnWidth(0);
    ui_.tableConns->setColumnWidth(0, 2 * defwidth);
    ui_.tableConns->setColumnWidth(1, 2 * defwidth);

    QObject::connect(ui_.tableConns->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(tableConns_selectionChanged()));

    QObject::connect(ui_.tableTasks->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(tableTasks_selectionChanged()));

    tableTasks_selectionChanged();
    tableConns_selectionChanged();
    DEBUG("Created downloads UI");
}


Downloads::~Downloads()
{
    DEBUG("Destroyed downloads UI");
}

void Downloads::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionConnect);
    menu.addAction(ui_.actionPreferSSL);
    menu.addAction(ui_.actionThrottle);
    menu.addSeparator();
    menu.addAction(ui_.actionTaskPause);
    menu.addAction(ui_.actionTaskResume);
    menu.addSeparator();
    menu.addAction(ui_.actionTaskMoveTop);
    menu.addAction(ui_.actionTaskMoveUp);
    menu.addAction(ui_.actionTaskMoveDown);
    menu.addAction(ui_.actionTaskMoveBottom);
    menu.addSeparator();
    menu.addAction(ui_.actionTaskDelete);
    menu.addAction(ui_.actionTaskClear);
}

void Downloads::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionConnect);
    bar.addAction(ui_.actionPreferSSL);
    bar.addAction(ui_.actionThrottle);
    bar.addSeparator();
    bar.addAction(ui_.actionTaskPause);
    bar.addAction(ui_.actionTaskResume);
    bar.addSeparator();
    bar.addAction(ui_.actionTaskMoveTop);
    bar.addAction(ui_.actionTaskMoveUp);
    bar.addAction(ui_.actionTaskMoveDown);
    bar.addAction(ui_.actionTaskMoveBottom);
    bar.addSeparator();
    bar.addAction(ui_.actionTaskDelete);
    bar.addAction(ui_.actionTaskClear);
}

void Downloads::loadState(app::Settings& s)
{
    const auto task_list_height = s.get("downloads", "task_list_height", 0);
    if (task_list_height)
         ui_.grpConns->setFixedHeight(task_list_height);

    //const auto enable_throttle = app::g_engine->get_
    const auto prefer_ssl      = app::g_engine->getPreferSecure();
    const auto connect         = app::g_engine->getConnect();
    const auto throttle        = app::g_engine->getThrottle();
    const auto group_related   = s.get("downloads", "group_related", false);
    const auto remove_complete = s.get("downloads", "remove_complete", false);

    ui_.actionConnect->setChecked(connect);
    ui_.actionThrottle->setChecked(throttle);
    ui_.actionPreferSSL->setChecked(prefer_ssl);
    ui_.chkGroupSimilar->setChecked(group_related);
    ui_.chkRemoveComplete->setChecked(remove_complete);
    tasks_.setGroupSimilar(group_related);
}

void Downloads::saveState(app::Settings& s)
{
    s.set("downloads", "task_list_height", ui_.grpConns->height());
    s.set("downloads", "group_related", ui_.chkGroupSimilar->isChecked());
    s.set("downloads", "remove_complete", ui_.chkRemoveComplete->isChecked());
}

void Downloads::refresh(bool isActive)
{
    const auto remove_complete = ui_.chkRemoveComplete->isChecked();
    //const auto group_similar   = ui_.chkGroupSimilar->isChecked();
    tasks_.refresh(remove_complete);
    conns_.refresh();

    const auto num_conns = conns_.rowCount(QModelIndex());
    const auto num_tasks = tasks_.rowCount(QModelIndex());
    ui_.grpTasks->setTitle(tr("Downloads (%1)").arg(num_tasks));
    ui_.grpConns->setTitle(tr("Connections (%1)").arg(num_conns));

    if (isActive)
        return;

    // only modify the title if we're not visible
    if (num_tasks > (int)numDownloads_)
    {
        const auto newTasks = num_tasks - numDownloads_;
        setWindowTitle(QString("Downloads (%1)").arg(newTasks));
    }
}

void Downloads::activate(QWidget*)
{
    const auto numTasks = tasks_.rowCount(QModelIndex());

    numDownloads_ = numTasks;
    setWindowTitle("Downloads");
}

void Downloads::updateRegistration(bool success)
{

}

void Downloads::on_actionConnect_triggered()
{
    const auto on_off = ui_.actionConnect->isChecked();

    app::g_engine->connect(on_off);
}

void Downloads::on_actionPreferSSL_triggered()
{
    const auto on_off = ui_.actionPreferSSL->isChecked();

    app::g_engine->setPreferSecure(on_off);
}

void Downloads::on_actionThrottle_triggered()
{
    const auto on_off = ui_.actionThrottle->isChecked();

    app::g_engine->setThrottle(on_off);
}

void Downloads::on_actionTaskPause_triggered()
{
    QModelIndexList indices = ui_.tableTasks->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    tasks_.pause(indices);

    tableTasks_selectionChanged();
}

void Downloads::on_actionTaskResume_triggered()
{
    QModelIndexList indices = ui_.tableTasks->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    tasks_.resume(indices);

    tableTasks_selectionChanged();
}

void Downloads::on_actionTaskMoveUp_triggered()
{
    QModelIndexList indices = ui_.tableTasks->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    tasks_.moveUp(indices);

    QItemSelection selection;
    for (int i=0; i<indices.size(); ++i)
        selection.select(indices[i], indices[i]);

    auto* model = ui_.tableTasks->selectionModel();
    model->setCurrentIndex(selection.indexes()[0],
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    model->select(selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void Downloads::on_actionTaskMoveDown_triggered()
{
    QModelIndexList indices = ui_.tableTasks->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    tasks_.moveDown(indices);

    QItemSelection selection;
    for (int i=0; i<indices.size(); ++i)
        selection.select(indices[i], indices[i]);

    auto* model = ui_.tableTasks->selectionModel();
    model->setCurrentIndex(selection.indexes()[0],
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    model->select(selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void Downloads::on_actionTaskMoveTop_triggered()
{
    QModelIndexList indices = ui_.tableTasks->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    tasks_.moveToTop(indices);

    QItemSelection selection;
    for (int i=0; i<indices.size(); ++i)
        selection.select(indices[i], indices[i]);

    auto* model = ui_.tableTasks->selectionModel();
    model->setCurrentIndex(selection.indexes()[0],
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    model->select(selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void Downloads::on_actionTaskMoveBottom_triggered()
{
    QModelIndexList indices = ui_.tableTasks->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    tasks_.moveToBottom(indices);

    QItemSelection selection;
    for (int i=0; i<indices.size(); ++i)
        selection.select(indices[i], indices[i]);

    auto* model = ui_.tableTasks->selectionModel();
    model->setCurrentIndex(selection.indexes()[0],
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    model->select(selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void Downloads::on_actionTaskDelete_triggered()
{
    QModelIndexList indices = ui_.tableTasks->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    tasks_.kill(indices);
}

void Downloads::on_actionTaskClear_triggered()
{
    tasks_.clear();
}

void Downloads::on_actionTaskOpenLog_triggered()
{
    const auto& file = app::g_engine->getEngineLogfile();
    app::openFile(file);
}

void Downloads::on_actionTaskOpenFolder_triggered()
{
    const QModelIndexList& indices = ui_.tableTasks->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    QSet<QString> opened;

    for (const auto& i : indices)
    {
        const auto& task = tasks_.getItem(i);
        if (task.path.empty())
        {
            const auto* acc = app::g_accounts->findAccount(task.account);
            if (acc == nullptr)
                continue;
            app::openFolder(acc->datapath);
            opened.insert(acc->datapath);
        }
        else
        {
            const auto& path = app::fromUtf8(task.path);
            if (opened.contains(path))
                continue;

            app::openFolder(path);
            opened.insert(path);
        }
    }
}

void Downloads::on_actionConnClone_triggered()
{
    QModelIndexList indices = ui_.tableConns->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    conns_.clone(indices);
}

void Downloads::on_actionConnDelete_triggered()
{
    QModelIndexList indices = ui_.tableConns->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    conns_.kill(indices);

    tableConns_selectionChanged();
}

void Downloads::on_actionConnOpenLog_triggered()
{
    QModelIndexList indices = ui_.tableConns->selectionModel()->selectedRows();
    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        const auto& item = conns_.getItem(row);
        const auto& file = app::fromUtf8(item.logfile);
        app::openFile(file);
    }
}

void Downloads::on_tableTasks_customContextMenuRequested(QPoint point)
{
    ui_.actionTaskOpenLog->setEnabled(app::g_engine->isStarted());

    QMenu menu(this);
    menu.addAction(ui_.actionTaskPause);
    menu.addAction(ui_.actionTaskResume);
    menu.addSeparator();
    menu.addAction(ui_.actionTaskMoveTop);
    menu.addAction(ui_.actionTaskMoveUp);
    menu.addAction(ui_.actionTaskMoveDown);
    menu.addAction(ui_.actionTaskMoveBottom);
    menu.addSeparator();
    menu.addAction(ui_.actionTaskDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionTaskClear);
    menu.addSeparator();
    menu.addAction(ui_.actionTaskOpenLog);
    menu.addSeparator();
    menu.addAction(ui_.actionTaskOpenFolder);
    menu.exec(QCursor::pos());
}

void Downloads::on_tableConns_customContextMenuRequested(QPoint point)
{
    QMenu menu(this);
    menu.addAction(ui_.actionConnect);
    menu.addAction(ui_.actionPreferSSL);
    menu.addAction(ui_.actionThrottle);
    menu.addSeparator();
    menu.addAction(ui_.actionConnClone);
    menu.addAction(ui_.actionConnDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionConnOpenLog);
    menu.exec(QCursor::pos());
}


void Downloads::on_tableConns_doubleClicked(const QModelIndex&)
{
    on_actionConnClone_triggered();
}

void Downloads::on_chkGroupSimilar_clicked(bool checked)
{
    tasks_.setGroupSimilar(checked);
}

void Downloads::on_lblRegister_linkActivated(QString)
{
    g_win->updateLicense();
}

void Downloads::on_lblDonate_linkActivated(QString)
{
    app::openWeb(NEWSFLASH_DONATE_URL);
}

void Downloads::tableTasks_selectionChanged()
{
    const auto& indices = ui_.tableTasks->selectionModel()->selectedRows();
    if (indices.isEmpty())
    {
        ui_.actionTaskPause->setEnabled(false);
        ui_.actionTaskResume->setEnabled(false);
        ui_.actionTaskMoveTop->setEnabled(false);
        ui_.actionTaskMoveUp->setEnabled(false);
        ui_.actionTaskMoveBottom->setEnabled(false);
        ui_.actionTaskMoveDown->setEnabled(false);
        ui_.actionTaskDelete->setEnabled(false);
        ui_.actionTaskOpenFolder->setEnabled(false);
        //ui_.actionTaskClear->setEnabled(false);
        return;
    }
    ui_.actionTaskPause->setEnabled(true);
    ui_.actionTaskResume->setEnabled(true);
    ui_.actionTaskMoveUp->setEnabled(true);
    ui_.actionTaskMoveTop->setEnabled(true);
    ui_.actionTaskMoveBottom->setEnabled(true);
    ui_.actionTaskMoveDown->setEnabled(true);
    ui_.actionTaskDelete->setEnabled(true);
    ui_.actionTaskOpenFolder->setEnabled(true);
    //ui_.actionTaskClear->setEnabled(true);

    using state = newsflash::ui::TaskDesc::States;

    const auto num_tasks = tasks_.rowCount(QModelIndex());

    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        const auto& ui = tasks_.getItem(row);

        // completed tasks cannot be paused.
        if (ui.state == state::Complete || ui.state == state::Error)
            ui_.actionTaskPause->setEnabled(false);

        // paused tasks cannot be paused again
        if (ui.state == state::Paused)
            ui_.actionTaskPause->setEnabled(false);

        // only paused tasks can be resumed, so if anything else but paused resume is not possible.
        if (ui.state != state::Paused)
            ui_.actionTaskResume->setEnabled(false);

        // if we're already at the top moving up is not possible
        if (row == 0)
        {
            ui_.actionTaskMoveTop->setEnabled(false);
            ui_.actionTaskMoveUp->setEnabled(false);
        }

        // if we're already at the bottom moving down is not possible
        if (row == num_tasks -1)
        {
            ui_.actionTaskMoveBottom->setEnabled(false);
            ui_.actionTaskMoveDown->setEnabled(false);
        }
    }

}

void Downloads::tableConns_selectionChanged()
{
    const auto& indices = ui_.tableConns->selectionModel()->selectedRows();
    if (indices.isEmpty())
    {
        ui_.actionConnClone->setEnabled(false);
        ui_.actionConnDelete->setEnabled(false);
        ui_.actionConnOpenLog->setEnabled(false);
    }
    else
    {
        ui_.actionConnClone->setEnabled(true);
        ui_.actionConnDelete->setEnabled(true);
        ui_.actionConnOpenLog->setEnabled(true);
    }
}

bool Downloads::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui_.splitter)
    {
        if (event->type() == QEvent::MouseMove)
        {
            const auto* mickey = static_cast<QMouseEvent*>(event);
            const auto& point  = mapFromGlobal(mickey->globalPos());
            const auto  ypos   = point.y();
            if (panels_y_pos_)
            {
                auto change = ypos - panels_y_pos_;
                auto height = ui_.grpTasks->height();
                if (height + change < 150)
                    return true;

                height = ui_.grpConns->height();
                if (height - change < 100)
                    return true;

                ui_.grpConns->setFixedHeight(height - change);
            }
            panels_y_pos_ = ypos;
            return true;
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            panels_y_pos_ = 0;
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

} // gui
