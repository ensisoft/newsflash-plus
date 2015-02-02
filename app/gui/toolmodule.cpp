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

ToolSettings::ToolSettings()
{
    ui_.setupUi(this);
}

ToolSettings::~ToolSettings()
{}

bool ToolSettings::validate() const 
{
    // we're always valid.
    return true;
}

void ToolSettings::setTools(std::vector<app::tools::tool> tools)
{
    tools_ = std::move(tools);

    for (const auto& tool : tools_)
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setIcon(tool.icon());
        item->setText(tool.name());
        ui_.listTools->addItem(item);
    }

    ui_.btnDel->setEnabled(!tools_.empty());
    ui_.btnEdit->setEnabled(!tools_.empty());
    ui_.btnMoveDown->setEnabled(tools_.size() > 1);
    ui_.btnMoveUp->setEnabled(tools_.size() > 1);
}

void ToolSettings::on_btnAdd_clicked()
{
    app::tools::tool tool;

    DlgTool dlg(this, tool);
    if (dlg.exec() == QDialog::Rejected)
        return;

    auto it = std::find_if(std::begin(tools_), std::end(tools_), 
        [&](const app::tools::tool& t) {
            return t.name() == tool.name();
        });
    if (it != std::end(tools_))
    {
        QMessageBox::critical(this, "Tool Already Exists", "A tool by that name already exists. Please use another name.");
        return;
    }

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

void ToolSettings::on_btnDel_clicked()
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

void ToolSettings::on_btnEdit_clicked()
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

void ToolSettings::on_btnMoveUp_clicked()
{
    int index = ui_.listTools->currentRow();
    if (index == -1 || index == 0)
        return;

    if (tools_.size() == 1)
        return;

    QListWidgetItem* item_current = ui_.listTools->item(index);
    QListWidgetItem* item_above   = ui_.listTools->item(index - 1);

    auto icon = item_current->icon();
    auto text = item_current->text();
    item_current->setIcon(item_above->icon());
    item_current->setText(item_above->text());
    item_above->setIcon(icon);
    item_above->setText(text);

    std::swap(tools_[index], tools_[index-1]);

    ui_.listTools->setCurrentRow(index - 1);
}

void ToolSettings::on_btnMoveDown_clicked()
{
    int index = ui_.listTools->currentRow();
    if (index == -1 || index == tools_.size() - 1)
        return;

    if (tools_.size() == 1)
        return;

    QListWidgetItem* item_current = ui_.listTools->item(index);
    QListWidgetItem* item_below   = ui_.listTools->item(index + 1);

    auto icon = item_current->icon();
    auto text = item_current->text();
    item_current->setIcon(item_below->icon());
    item_current->setText(item_below->text());
    item_below->setIcon(icon);
    item_below->setText(text);

    std::swap(tools_[index], tools_[index+1]);

    ui_.listTools->setCurrentRow(index + 1);
}

void ToolSettings::on_listTools_doubleClicked(QModelIndex)
{
    on_btnEdit_clicked();
}

ToolModule::ToolModule()
{}

ToolModule::~ToolModule()
{}

SettingsWidget* ToolModule::getSettings()
{
    auto* ptr = new ToolSettings;

    ptr->setTools(app::g_tools->get_tools_copy());

    return ptr;
}


void ToolModule::applySettings(SettingsWidget* gui)
{
    auto* ptr = dynamic_cast<ToolSettings*>(gui);

    app::g_tools->set_tools_copy(ptr->tools_);
}

void ToolModule::freeSettings(SettingsWidget* gui)
{
    delete gui;
}


} // gui
