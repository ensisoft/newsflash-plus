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

#define LOGTAG "extract"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QFileDialog>
#include <newsflash/warnpop.h>
#include "unpack.h"
#include "../debug.h"
#include "../settings.h"
#include "../archive.h"
#include "../unpacker.h"
#include "../utility.h"
#include "../platform.h"

namespace gui 
{

Unpack::Unpack(app::Unpacker& unpacker) : model_(unpacker), numUnpacks_(0)
{
    ui_.setupUi(this);
    ui_.unpackList->setModel(unpacker.getUnpackList());
    ui_.unpackData->setModel(unpacker.getUnpackData());
    ui_.progressBar->setVisible(false);
    ui_.progressBar->setMinimum(0);
    ui_.progressBar->setMaximum(100);
    ui_.actionClear->setEnabled(false);
    ui_.actionStop->setEnabled(false);
    ui_.lblStatus->clear();

    QObject::connect(&model_, SIGNAL(unpackStart(const app::Archive&)),
        this, SLOT(unpackStart(const app::Archive&)));
    QObject::connect(&model_, SIGNAL(unpackReady(const app::Archive&)),
        this, SLOT(unpackReady(const app::Archive&)));
    QObject::connect(&model_, SIGNAL(unpackProgress(const QString&, int)),
        this, SLOT(unpackProgress(const QString&, int)));

    QObject::connect(ui_.unpackList->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(unpackList_selectionChanged()));

    unpackList_selectionChanged();

    DEBUG("Unpack UI created");
}

Unpack::~Unpack()
{
    DEBUG("Unpack UI deleted");
}

void Unpack::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionUnpack);
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

void Unpack::addActions(QMenu& menu)
{
    // no menu actions.
}

void Unpack::activate(QWidget*) 
{
    numUnpacks_ = 0;
    setWindowTitle("Extract");
}

void Unpack::deactivate()
{
    numUnpacks_ = 0;
    setWindowTitle("Extract");
}

void Unpack::refresh(bool isActive)
{
    if (!isActive && numUnpacks_)
        setWindowTitle(QString("Extract (%1)").arg(numUnpacks_));
}

void Unpack::loadState(app::Settings& settings)
{
    app::loadTableLayout("unpack", ui_.unpackList, settings);
    app::loadTableLayout("unpack", ui_.unpackData, settings);


    const auto keepBroken = settings.get("unpack", "keep_broken", true);
    const auto overwrite  = settings.get("unpack", "overwrite_existing_files", false);
    const auto purge      = settings.get("unpack", "purge_on_success", true);
    const auto writeLog   = settings.get("unpack", "write_log", true);

    ui_.chkKeepBroken->setChecked(keepBroken);
    ui_.chkOverwriteExisting->setChecked(keepBroken);
    ui_.chkPurge->setChecked(purge);    
    ui_.chkWriteLog->setChecked(writeLog);

    model_.setPurgeOnSuccess(purge);
    model_.setKeepBroken(keepBroken);
    model_.setOverwriteExisting(overwrite);
}

void Unpack::saveState(app::Settings& settings)
{
    app::saveTableLayout("unpack", ui_.unpackList, settings);
    app::saveTableLayout("unpack", ui_.unpackData, settings);

    const auto keepBroken = ui_.chkKeepBroken->isChecked();
    const auto overwrite  = ui_.chkOverwriteExisting->isChecked();
    const auto purge      = ui_.chkPurge->isChecked();
    const auto writeLog   = ui_.chkWriteLog->isChecked();
    settings.set("unpack", "keep_broken", keepBroken);
    settings.set("unpack", "overwrite_existing_files", overwrite);
    settings.set("unpack", "purge_on_success", purge);
    settings.set("unpack", "write_log", writeLog);
}

void Unpack::shutdown()
{
    model_.stopUnpack();
}

void Unpack::setUnpackEnabled(bool onOff)
{
    model_.setEnabled(onOff);
}

void Unpack::on_unpackList_customContextMenuRequested(QPoint)
{
    QMenu menu(this);
    menu.addAction(ui_.actionUnpack);
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
    //menu.addAction(ui_.actionDetails);
    menu.exec(QCursor::pos());
}

void Unpack::unpackList_selectionChanged()
{
    auto indices = ui_.unpackList->selectionModel()->selectedRows();

    ui_.actionOpenFolder->setEnabled(false);
    ui_.actionOpenLog->setEnabled(false);    
    ui_.actionTop->setEnabled(false);
    ui_.actionMoveUp->setEnabled(false);
    ui_.actionMoveDown->setEnabled(false);
    ui_.actionBottom->setEnabled(false);
    ui_.actionDelete->setEnabled(false);
    ui_.actionStop->setEnabled(false);
    if (indices.isEmpty())
        return;

    qSort(indices);
    const auto first = indices.first();
    const auto last  = indices.last();
    const auto firstRow = first.row();
    const auto lastRow  = last.row();
    const auto numRows  = (int)model_.numUnpacks();

    ui_.actionTop->setEnabled(firstRow > 0);
    ui_.actionMoveUp->setEnabled(firstRow > 0);
    ui_.actionMoveDown->setEnabled(lastRow < numRows - 1);
    ui_.actionBottom->setEnabled(lastRow < numRows - 1);
    ui_.actionDelete->setEnabled(true);
    ui_.actionOpenLog->setEnabled(true);
    ui_.actionOpenFolder->setEnabled(true);

    ui_.actionStop->setEnabled(true);
    for (const auto i : indices)
    {
        const auto& item = model_.getUnpack(i);
        if (item.state != app::Archive::Status::Active)
        {
            ui_.actionStop->setEnabled(false);
            break;
        }
    }
}

void Unpack::on_actionUnpack_triggered()
{
    const auto& file = QFileDialog::getOpenFileName(this,
        tr("Select Archive File"), QString(), "(Archive FIles ) *.rar");
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
    model_.addUnpack(arc);
}

void Unpack::on_actionStop_triggered()
{
    model_.stopUnpack();
}

void Unpack::on_actionOpenFolder_triggered()
{
    const auto& indices = ui_.unpackList->selectionModel()->selectedRows();
    for (const auto& i : indices)
    {
        const auto& item = model_.getUnpack(i);
        app::openFile(item.path);
    }
}

void Unpack::on_actionTop_triggered()
{
    moveTasks(MoveDirection::Top);
}

void Unpack::on_actionMoveUp_triggered()
{
    moveTasks(MoveDirection::Up);
}

void Unpack::on_actionMoveDown_triggered()
{
    moveTasks(MoveDirection::Down);
}

void Unpack::on_actionBottom_triggered()
{
    moveTasks(MoveDirection::Bottom);
}

void Unpack::on_actionClear_triggered()
{
    model_.killComplete();

    unpackList_selectionChanged();

    ui_.actionClear->setEnabled(false);
}

void Unpack::on_actionDelete_triggered()
{
    auto indices = ui_.unpackList->selectionModel()->selectedRows();

    model_.kill(indices);

    unpackList_selectionChanged();
}

void Unpack::on_actionOpenLog_triggered()
{
    auto indices =  ui_.unpackList->selectionModel()->selectedRows();

    model_.openLog(indices);
}

void Unpack::on_chkWriteLog_stateChanged(int)
{
    model_.setWriteLog(ui_.chkWriteLog->isChecked());
}
void Unpack::on_chkOverwriteExisting_stateChanged(int)
{
    model_.setOverwriteExisting(ui_.chkOverwriteExisting->isChecked());
}

void Unpack::on_chkPurge_stateChanged(int)
{
    model_.setPurgeOnSuccess(ui_.chkPurge->isChecked());
}

void Unpack::on_chkKeepBroken_stateChanged(int)
{
    model_.setKeepBroken(ui_.chkKeepBroken->isChecked());
}

void Unpack::unpackStart(const app::Archive& arc)
{
    ui_.progressBar->setVisible(true);
    ui_.actionStop->setEnabled(true);
    ui_.lblStatus->setVisible(true);

    numUnpacks_++;
}

void Unpack::unpackReady(const app::Archive& arc)
{
    ui_.progressBar->setVisible(false);
    ui_.actionStop->setEnabled(false);
    ui_.actionClear->setEnabled(true);
    ui_.lblStatus->setVisible(false);
}

void Unpack::unpackProgress(const QString& target, int done)
{
    ui_.progressBar->setValue(done);    
    ui_.lblStatus->setText(target);
    ui_.unpackData->scrollToBottom();
}

void Unpack::moveTasks(MoveDirection direction)
{
    auto* model = ui_.unpackList->selectionModel();

    auto indices = model->selectedRows();

    switch (direction)
    {
        case MoveDirection::Up:
            model_.moveUp(indices);
            break;
        case MoveDirection::Down:
            model_.moveDown(indices);
            break;
        case MoveDirection::Top:
            model_.moveToTop(indices);
            break;
        case MoveDirection::Bottom:
            model_.moveToBottom(indices);
            break;
    }

    QItemSelection selection;
    for (int i=0; i<indices.size(); ++i)
        selection.select(indices[i], indices[i]);

    model->setCurrentIndex(selection.indexes().first(),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    model->select(selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

} // gui
