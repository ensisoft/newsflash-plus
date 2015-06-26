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

#define LOGTAG "repair"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QFileDialog>
#include <newsflash/warnpop.h>
#include "dlgarchive.h"
#include "repair.h"
#include "../debug.h"
#include "../settings.h"
#include "../repairer.h"
#include "../archive.h"
#include "../utility.h"
#include "../platform.h"

namespace gui 
{

Repair::Repair(app::Repairer& repairer) : model_(repairer), numRepairs_(0)
{
    ui_.setupUi(this);
    ui_.repairList->setModel(model_.getRecoveryList());
    ui_.repairData->setModel(model_.getRecoveryData());
    ui_.progressList->setVisible(false);
    ui_.progressList->setMinimum(0);
    ui_.progressList->setMaximum(100);    
    ui_.actionStop->setEnabled(false);
    ui_.actionClear->setEnabled(false);
    ui_.lblStatus->clear();    

    QObject::connect(&model_, SIGNAL(repairStart(const app::Archive&)),
        this, SLOT(repairStart(const app::Archive&)));
    QObject::connect(&model_, SIGNAL(repairReady(const app::Archive&)),
        this, SLOT(repairReady(const app::Archive&)));

    QObject::connect(&model_, SIGNAL(scanProgress(const QString&, int)),
        this, SLOT(scanProgress(const QString&, int)));
    QObject::connect(&model_, SIGNAL(repairProgress(const QString&, int)),
        this, SLOT(repairProgress(const QString&, int)));

    QObject::connect(ui_.repairList->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(repairList_selectionChanged()));

    repairList_selectionChanged();

    DEBUG("Repair UI created");
}

Repair::~Repair()
{
    DEBUG("Repair UI deleted");
}

void Repair::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionRepair);
    bar.addSeparator();    
    bar.addAction(ui_.actionOpenFolder);
    bar.addSeparator();
    bar.addAction(ui_.actionTop);
    bar.addAction(ui_.actionMoveUp);
    bar.addAction(ui_.actionMoveDown);
    bar.addAction(ui_.actionBottom);
    bar.addSeparator();                
    bar.addAction(ui_.actionDelete);    
    bar.addAction(ui_.actionClear);    
    bar.addSeparator();
    bar.addAction(ui_.actionStop);
}

void Repair::addActions(QMenu& menu)
{
    // no actions atm.
}


void Repair::activate(QWidget*)
{
    numRepairs_ = 0;
    setWindowTitle("Repairs");
}

void Repair::deactivate()
{
    numRepairs_ = 0;
    setWindowTitle("Repairs");
}

void Repair::refresh(bool isActive)
{
    if (!isActive && numRepairs_)
        setWindowTitle(QString("Repairs (%1)").arg(numRepairs_));    
}

void Repair::loadState(app::Settings& settings)
{
    app::loadTableLayout("repair", ui_.repairList, settings);
    app::loadTableLayout("repair", ui_.repairData, settings);

    const auto writeLogs = settings.get("repair", "write_log_files", true);
    const auto purgePars = settings.get("repair", "purge_recovery_files_on_success", false);
    ui_.chkWriteLogs->setChecked(writeLogs);
    ui_.chkPurgePars->setChecked(purgePars);

    model_.setPurgePars(purgePars);
    model_.setWriteLogs(writeLogs);
}

void Repair::saveState(app::Settings& settings)
{
    app::saveTableLayout("repair", ui_.repairList, settings);
    app::saveTableLayout("repair", ui_.repairData, settings);

    const auto writeLogs = ui_.chkWriteLogs->isChecked();
    const auto purgePars = ui_.chkPurgePars->isChecked();
    settings.set("repair", "write_log_files", writeLogs);
    settings.set("repair", "purge_recovery_files_on_success", purgePars);
}

void Repair::shutdown()
{
    model_.stopRecovery();
}

void Repair::setRepairEnabled(bool onOff)
{
    model_.setEnabled(onOff);
}

void Repair::on_repairList_customContextMenuRequested(QPoint)
{
    QMenu menu(this);
    menu.addAction(ui_.actionRepair);
    menu.addSeparator();
    menu.addAction(ui_.actionOpenFolder);
    menu.addSeparator();
    menu.addAction(ui_.actionTop);
    menu.addAction(ui_.actionMoveUp);
    menu.addAction(ui_.actionMoveDown);
    menu.addAction(ui_.actionBottom);
    menu.addSeparator();    
    menu.addAction(ui_.actionDelete);    
    menu.addSeparator();
    menu.addAction(ui_.actionStop);
    menu.addSeparator();
    menu.addAction(ui_.actionOpenLog);
    menu.addAction(ui_.actionDetails);
    menu.exec(QCursor::pos());
}

void Repair::on_actionRepair_triggered()
{
    const auto& file = QFileDialog::getOpenFileName(this,
        tr("Select Recovery File"), QString(), "(Recovery Files) *.par2");
    if (file.isEmpty())
        return;

    QFileInfo info(file);
    auto base = info.completeBaseName();
    auto name = info.fileName();
    auto path = info.filePath().remove(name);

    app::Archive arc;
    arc.state = app::Archive::Status::Queued;
    arc.desc  = name;
    arc.file  = name;
    arc.path  = path;
    model_.addRecovery(arc);
}

void Repair::on_actionDelete_triggered()
{
    auto indices = ui_.repairList->selectionModel()->selectedRows();

    model_.kill(indices);

    repairList_selectionChanged();    
}

void Repair::on_actionMoveUp_triggered()
{
    moveTasks(TaskDirection::Up);
}

void Repair::on_actionMoveDown_triggered()
{
    moveTasks(TaskDirection::Down);
}

void Repair::on_actionTop_triggered()
{
    moveTasks(TaskDirection::Top);
}

void Repair::on_actionBottom_triggered()
{
    moveTasks(TaskDirection::Bottom);
}

void Repair::on_actionClear_triggered()
{
    model_.killComplete();

    repairList_selectionChanged();

    ui_.actionClear->setEnabled(false);
}

void Repair::on_actionOpenLog_triggered()
{
    auto indices = ui_.repairList->selectionModel()->selectedRows();

    model_.openLog(indices);
}

void Repair::on_actionStop_triggered()
{
    model_.stopRecovery();
}

void Repair::on_actionOpenFolder_triggered()
{
    const auto& indices = ui_.repairList->selectionModel()->selectedRows();
    for (const auto& i : indices)
    {
        const auto& item = model_.getRecovery(i);
        app::openFile(item.path);
    }
}

void Repair::on_actionDetails_triggered()
{
    const auto& indices = ui_.repairList->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;
    const auto& first = indices.first();
    const auto& arc   = model_.getRecovery(first);
    auto* data  = model_.getRecoveryData(first);
    DlgArchive dlg(this, data);
    dlg.setWindowTitle(arc.file);
    dlg.exec();
}

void Repair::on_chkWriteLogs_stateChanged(int)
{
    const auto value = ui_.chkWriteLogs->isChecked();

    model_.setWriteLogs(value);
}

void Repair::on_chkPurgePars_stateChanged(int)
{
    const auto value = ui_.chkPurgePars->isChecked();

    model_.setPurgePars(value);
}

void Repair::repairList_selectionChanged()
{
    auto indices = ui_.repairList->selectionModel()->selectedRows();

    ui_.actionOpenFolder->setEnabled(false);
    ui_.actionTop->setEnabled(false);
    ui_.actionMoveUp->setEnabled(false);
    ui_.actionMoveDown->setEnabled(false);
    ui_.actionBottom->setEnabled(false);
    ui_.actionDelete->setEnabled(false);
    ui_.actionOpenLog->setEnabled(false);
    ui_.actionDetails->setEnabled(false);
    if (indices.isEmpty())
        return;

    qSort(indices);
    const auto first = indices.first();
    const auto last  = indices.last();
    const auto firstRow = first.row();
    const auto lastRow  = last.row();
    const auto numRows = (int)model_.numRepairs();    

    ui_.actionTop->setEnabled(firstRow > 0);
    ui_.actionMoveUp->setEnabled(firstRow > 0);
    ui_.actionMoveDown->setEnabled(lastRow < numRows - 1);
    ui_.actionBottom->setEnabled(lastRow < numRows - 1);
    ui_.actionDelete->setEnabled(true);
    ui_.actionOpenLog->setEnabled(true);
    ui_.actionOpenFolder->setEnabled(true);
    ui_.actionDetails->setEnabled(true);

    ui_.actionStop->setEnabled(true);
    for (const auto& i : indices)
    {
        const auto& item = model_.getRecovery(i);
        if (item.state != app::Archive::Status::Active)
        {
            ui_.actionStop->setEnabled(false);
            break;
        }
    }
}

void Repair::repairStart(const app::Archive& rec)
{
    ui_.progressList->setVisible(true);
    ui_.lblStatus->setVisible(true);
    if (ui_.chkWriteLogs->isChecked())
        ui_.actionOpenLog->setChecked(true);

    numRepairs_++;
}

void Repair::repairReady(const app::Archive& rec)
{
    ui_.progressList->setVisible(false);
    ui_.lblStatus->setVisible(false);
    ui_.actionClear->setEnabled(true);
}

void Repair::scanProgress(const QString& file, int done)
{
    ui_.progressList->setValue(done);
    ui_.lblStatus->setText(QString("Scanning ... %1").arg(file));
    ui_.repairData->scrollToBottom();
}

void Repair::repairProgress(const QString& step, int done)
{
    ui_.progressList->setValue(done);
    ui_.lblStatus->setText(step);
}

void Repair::moveTasks(TaskDirection dir)
{
    auto indices = ui_.repairList->selectionModel()->selectedRows();

    switch (dir)
    {
        case TaskDirection::Up:
            model_.moveUp(indices);
            break;
        case TaskDirection::Down:
            model_.moveDown(indices);
            break;
        case TaskDirection::Top:
            model_.moveToTop(indices);
            break;
        case TaskDirection::Bottom:
            model_.moveToBottom(indices);
            break;
    }


    QItemSelection selection;
    for (int i=0; i<indices.size(); ++i)
        selection.select(indices[i], indices[i]);

    auto* model = ui_.repairList->selectionModel();
    model->setCurrentIndex(selection.indexes().first(),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    model->select(selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

} // gui
