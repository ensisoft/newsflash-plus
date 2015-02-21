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
#include <newsflash/warnpop.h>
#include "extract.h"
#include "../debug.h"
#include "../settings.h"
//#include "../extract.h"

namespace gui 
{

Extract::Extract()
{
    ui_.setupUi(this);
    //ui_.tableView->setModel(app::g_repair);
    DEBUG("Extract GUI created");
}

Extract::~Extract()
{
    DEBUG("Extract GUI deleted");
}

void Extract::addActions(QToolBar& bar)
{
    // bar.addAction(ui_.actionRun);
    // bar.addSeparator();
    // bar.addAction(ui_.actionStop);
}

void Extract::addActions(QMenu& menu)
{
    // menu.addAction(ui_.actionRun);
    // menu.addSeparator();
    // menu.addAction(ui_.actionStop);
}

void Extract::loadState(app::Settings& settings)
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

void Extract::saveState(app::Settings& settings)
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

} // gui
