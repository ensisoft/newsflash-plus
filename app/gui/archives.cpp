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

#define LOGTAG "unpack"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QFileDialog>
#include <newsflash/warnpop.h>
#include "archives.h"
#include "../debug.h"
#include "../settings.h"

using app::str;

namespace gui 
{

Archives::Archives()
{
    ui_.setupUi(this);
    ui_.tableList->setModel(app::g_unpacker->getUnpackList());
    ui_.tableData->setModel(app::g_unpacker->getUnpackData());
    ui_.progressBar->setVisible(false);
    ui_.progressBar->setMinimum(0);
    ui_.progressBar->setMaximum(100);
    ui_.lblStatus->clear();

    QObject::connect(app::g_unpacker, SIGNAL(unpackStart(const app::Archive&)),
        this, SLOT(unpackStart(const app::Archive&)));
    QObject::connect(app::g_unpacker, SIGNAL(unpackReady(const app::Archive&)),
        this, SLOT(unpackReady(const app::Archive&)));

    QObject::connect(app::g_unpacker, SIGNAL(unpackProgress(int)),
        this, SLOT(unpackProgress(int)));        

    DEBUG("Archives UI created");
}

Archives::~Archives()
{
    DEBUG("Archives UI deleted");
}

void Archives::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionUnpack);
    bar.addSeparator();    
    bar.addAction(ui_.actionStop);
}

void Archives::addActions(QMenu& menu)
{
    // menu.addAction(ui_.actionRun);
    // menu.addSeparator();
    // menu.addAction(ui_.actionStop);
}

void Archives::loadState(app::Settings& settings)
{
    // const auto* queue = ui_.tableView->model();
    // for (int i=0; i<queue->columnCount()-1; ++i)
    // {
    //     const auto name  = QString("queue_col_%1").arg(i);
    //     const auto width = settings.get("scripts", name, 
    //         ui_.tableView->columnWidth(i));
    //     ui_.tableView->setColumnWidth(i, width);
    // }    

    // const auto writeLog  = settings.get("scripts", "write_log_files", true);
    // const auto followOut = settings.get("scripts", "follow_output", false);
    // ui_.chkWriteLog->setChecked(writeLog);
    // ui_.chkFollow->setChecked(followOut);
}

void Archives::saveState(app::Settings& settings)
{
    // const auto* queue = ui_.tableView->model();
    // for (int i=0; i<queue->columnCount()-1; ++i)
    // {
    //     const auto width = ui_.tableView->columnWidth(i);
    //     const auto name  = QString("queue_col_%1").arg(i);
    //     settings.set("scripts", name, width);
    // }    


    // const auto writeLog  = ui_.chkWriteLog->isChecked();
    // const auto followOut = ui_.chkFollow->isChecked();
    // settings.set("scripts", "write_log_files", writeLog);
    // settings.set("scripts", "follow_output", followOut);
}

void Archives::on_actionUnpack_triggered()
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
    app::g_unpacker->addUnpack(arc);
}

void Archives::on_actionStop_triggered()
{
    app::g_unpacker->stopUnpack();
}

void Archives::unpackStart(const app::Archive& arc)
{
    ui_.progressBar->setVisible(true);
    ui_.actionStop->setEnabled(true);
    ui_.lblStatus->setVisible(true);
}

void Archives::unpackReady(const app::Archive& arc)
{
    ui_.progressBar->setVisible(false);
    ui_.actionStop->setEnabled(false);
    ui_.lblStatus->setVisible(false);
}

void Archives::unpackProgress(int done)
{
    ui_.progressBar->setValue(done);    
    //ui_.lblStatus->setText(file);
    ui_.tableData->scrollToBottom();
}

} // gui
