// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#define LOGTAG "repair"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QFileDialog>
#include <newsflash/warnpop.h>
#include "repair.h"
#include "../debug.h"
#include "../settings.h"
#include "../repairer.h"
#include "../archive.h"

namespace gui 
{

Repair::Repair(app::Repairer& repairer) : model_(repairer), numRepairs_(0)
{
    ui_.setupUi(this);
    ui_.tableList->setModel(model_.getRecoveryList());
    ui_.tableData->setModel(model_.getRecoveryData());
    ui_.progressList->setVisible(false);
    ui_.progressList->setMinimum(0);
    ui_.progressList->setMaximum(100);    
    ui_.actionStop->setEnabled(false);
    ui_.lblStatus->clear();    

    QObject::connect(&model_, SIGNAL(recoveryStart(const app::Archive&)),
        this, SLOT(recoveryStart(const app::Archive&)));
    QObject::connect(&model_, SIGNAL(recoveryReady(const app::Archive&)),
        this, SLOT(recoveryReady(const app::Archive&)));

    QObject::connect(&model_, SIGNAL(scanProgress(const QString&, int)),
        this, SLOT(scanProgress(const QString&, int)));
    QObject::connect(&model_, SIGNAL(repairProgress(const QString&, int)),
        this, SLOT(repairProgress(const QString&, int)));

    DEBUG("Repair UI created");
}

Repair::~Repair()
{
    DEBUG("Repair UI deleted");
}

void Repair::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionAutoRepair);
    bar.addSeparator();
    bar.addAction(ui_.actionAdd);
    bar.addAction(ui_.actionDel);
    bar.addSeparator();        
    bar.addAction(ui_.actionStop);
}

void Repair::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionAutoRepair);
    menu.addSeparator();
    menu.addAction(ui_.actionStop);
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
    const auto* queue = ui_.tableList->model();
    for (int i=0; i<queue->columnCount()-1; ++i)
    {
        const auto name  = QString("queue_col_%1").arg(i);
        const auto width = settings.get("scripts", name, 
            ui_.tableList->columnWidth(i));
        ui_.tableList->setColumnWidth(i, width);
    }    

    // const auto writeLog  = settings.get("scripts", "write_log_files", true);
    // const auto followOut = settings.get("scripts", "follow_output", false);
    // ui_.chkWriteLog->setChecked(writeLog);
    // ui_.chkFollow->setChecked(followOut);
}

void Repair::saveState(app::Settings& settings)
{
    const auto* queue = ui_.tableList->model();
    for (int i=0; i<queue->columnCount()-1; ++i)
    {
        const auto width = ui_.tableList->columnWidth(i);
        const auto name  = QString("queue_col_%1").arg(i);
        settings.set("scripts", name, width);
    }    

    // const auto writeLog  = ui_.chkWriteLog->isChecked();
    // const auto followOut = ui_.chkFollow->isChecked();
    // settings.set("scripts", "write_log_files", writeLog);
    // settings.set("scripts", "follow_output", followOut);
}


void Repair::on_tableView_customContextMenuRequested(QPoint)
{

}

void Repair::on_actionRecover_triggered()
{}

void Repair::on_actionStop_triggered()
{
    model_.stopRecovery();
}

void Repair::on_actionAdd_triggered()
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

void Repair::on_actionDel_triggered()
{

}

void Repair::recoveryStart(const app::Archive& rec)
{
    ui_.progressList->setVisible(true);
    ui_.lblStatus->setVisible(true);
    ui_.actionStop->setEnabled(true);

    numRepairs_++;
}

void Repair::recoveryReady(const app::Archive& rec)
{
    ui_.progressList->setVisible(false);
    ui_.lblStatus->setVisible(false);
    ui_.actionStop->setEnabled(false);
}

void Repair::scanProgress(const QString& file, int done)
{
    ui_.progressList->setValue(done);
    ui_.lblStatus->setText(QString("Scanning ... %1").arg(file));
    ui_.tableData->scrollToBottom();
}

void Repair::repairProgress(const QString& step, int done)
{
    ui_.progressList->setValue(done);
    ui_.lblStatus->setText(step);
}

} // gui
