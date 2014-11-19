// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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

#define LOGTAG "tools"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QFileDialog>
#  include <QtGui/QMessageBox>
#  include <QDir>
#include <newsflash/warnpop.h>

#include "dlgtool.h"
#include "toolmodule.h"
#include "mainwindow.h"
#include "../tools.h"

namespace gui 
{

toolsettings::toolsettings()
{
    ui_.setupUi(this);

    for (const auto& tool : tools_)
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setIcon(tool.icon());
        item->setText(tool.name());
        ui_.listTools->addItem(item);
    }

    const auto num_tools = app::g_tools->num_tools();
    ui_.btnDel->setEnabled(num_tools != 0);
    ui_.btnEdit->setEnabled(num_tools != 0);
    ui_.btnMoveDown->setEnabled(num_tools > 1);
    ui_.btnMoveUp->setEnabled(num_tools > 1);
}

toolsettings::~toolsettings()
{}

bool toolsettings::validate() const 
{
    // we're always valid.
    return true;
}

void toolsettings::on_btnAdd_clicked()
{
    app::tools::tool tool;

    DlgTool dlg(this, tool);
    if (dlg.exec() == QDialog::Rejected)
        return;

    tools_.push_back(tool);

    QListWidgetItem* item = new QListWidgetItem();
    item->setIcon(tool.icon());
    item->setText(tool.name());
    ui_.listTools->addItem(item);

    ui_.btnDel->setEnabled(true);
    ui_.btnEdit->setEnabled(true);

    const auto num_tools = tools_.size();
    if (num_tools > 1)
    {
        ui_.btnMoveDown->setEnabled(true);
        ui_.btnMoveUp->setEnabled(true);
    }

    if (ui_.listTools->currentRow() == -1)
        ui_.listTools->setCurrentRow(0);
}

void toolsettings::on_btnDel_clicked()
{
    const auto row  = ui_.listTools->currentRow();
    if (row == -1)
        return;
    const auto item = ui_.listTools->takeItem(row);
    delete item;

    auto it = tools_.begin();
    std::advance(it, row);
    tools_.erase(it);

    const auto num_tools = tools_.size();

    if (num_tools == 0)
    {
        ui_.btnDel->setEnabled(false);
        ui_.btnEdit->setEnabled(false);
    }
    if (num_tools < 2)
    {
        ui_.btnMoveDown->setEnabled(false);
        ui_.btnMoveUp->setEnabled(false);
    }
}

void toolsettings::on_btnEdit_clicked()
{
    const auto row = ui_.listTools->currentRow();
    if (row == -1)
        return;

    auto& tool = tools_[row];

    DlgTool dlg(this, tool);
    if (dlg.exec() == QDialog::Rejected)
        return;

    QListWidgetItem* item = ui_.listTools->item(row);
    item->setIcon(tool.icon());
    item->setText(tool.name());
    ui_.listTools->editItem(item);
}

void toolsettings::on_listTools_doubleClicked(QModelIndex)
{
    on_btnEdit_clicked();
}

toolmodule::toolmodule()
{}

toolmodule::~toolmodule()
{}

gui::settings* toolmodule::get_settings(app::settings& backend)
{
    auto* ptr = new toolsettings;

    ptr->tools_ = app::g_tools->get_tools_copy();

    return ptr;
}


void toolmodule::apply_settings(settings* gui, app::settings& backend)
{
    auto* ptr = dynamic_cast<toolsettings*>(gui);

    app::g_tools->set_tools_copy(ptr->tools_);
}

void toolmodule::free_settings(settings* gui)
{
    delete gui;
}


} // gui
