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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMessageBox>
#  include <QtGui/QMenu>
#include <newsflash/warnpop.h>
#include "dlgvolumes.h"
#include "../newsgroup.h"
#include "../format.h"

namespace gui
{


DlgVolumes::DlgVolumes(QWidget* parent, app::NewsGroup& model) : QDialog(parent), model_(model)
{
    ui_.setupUi(this);
    ui_.tableView->setModel(model_.getVolumeList());
}

void DlgVolumes::on_btnClose_clicked()
{
    close();
}

void DlgVolumes::on_btnPurge_clicked()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    for (const auto& i : indices)
        model_.purge(i.row());
}

void DlgVolumes::on_btnLoad_clicked()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    try 
    {
        for (const auto& i : indices)
            model_.load(i.row());
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, "Headers", 
            tr("Unable to load the newsgroup data.\n%1").arg(app::widen(e.what())));
    }
}

void DlgVolumes::on_actionLoad_triggered()
{
    on_btnLoad_clicked();
}

void DlgVolumes::on_actionPurge_triggered()
{
    on_btnPurge_clicked();
}

void DlgVolumes::on_tableView_customContextMenuRequested(QPoint)
{
    QMenu menu(this);
    menu.addAction(ui_.actionLoad);
    menu.addSeparator();
    menu.addAction(ui_.actionPurge);
    menu.exec(QCursor::pos());
}



} // gui