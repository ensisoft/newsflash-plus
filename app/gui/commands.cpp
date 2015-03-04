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
#  include <QtGui/QMenu>
#  include <QAbstractTableModel>
#include <newsflash/warnpop.h>
#include <algorithm>
#include "commands.h"
#include "dlgcommand.h"
#include "../utility.h"
#include "../debug.h"

namespace gui
{

class CmdSettings::Model : public QAbstractTableModel
{
public:
    Model(app::Commands::CmdList cmds) : commands_(cmds) 
    {}

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal)
            return {};

        if (role == Qt::DisplayRole)
        {
            switch ((Columns)section)
            {
                case Columns::Enabled: return "";
                case Columns::Exec:    return "Executable";
                case Columns::Comment: return "Comment";                
                case Columns::LAST:    Q_ASSERT(0);
            }
        }
        if (role == Qt::DecorationRole)
        {
            if ((Columns)section == Columns::Enabled)
            {
                static const QIcon bullet(app::toGrayScale("icons:ico_bullet_green.png"));
                return bullet;
            }

        }
        return {};
    }

    virtual QVariant data(const QModelIndex& index, int role) const override
    {
        const auto row = index.row();
        const auto col = index.column();

        const auto& script = commands_[row];

        if (role == Qt::DisplayRole)
        {
            switch ((Columns)col)
            {
                case Columns::Enabled: return "";
                case Columns::Exec:    return script.exec();
                case Columns::Comment: return script.comment();
                case Columns::LAST:    Q_ASSERT(0);
            }
        }
        if (role == Qt::DecorationRole)
        {
            if ((Columns)col == Columns::Exec)
                return script.icon();
            if ((Columns)col == Columns::Enabled)
            {
                if (script.isEnabled())
                    return QIcon("icons:ico_bullet_green.png");
                return QIcon("icons:ico_bullet_red.png");
            }
        }
        return {};
    }

    virtual int rowCount(const QModelIndex&) const override
    {
        return (int)commands_.size();
    }
    virtual int columnCount(const QModelIndex&) const override
    {
        return (int)Columns::LAST;
    }

    app::Commands::Command& getCommand(std::size_t index)
    { 
        Q_ASSERT(index < commands_.size());
        return commands_[index]; 
    }

    app::Commands::CmdList getCommands()
    { return commands_; }

    void delCommand(std::size_t index)
    {
        BOUNDSCHECK(commands_, index);
        auto it = std::begin(commands_);
        std::advance(it, index);
        commands_.erase(it);

        refresh();
    }

    void setCommand(std::size_t index, const app::Commands::Command& cmd)
    {
        BOUNDSCHECK(commands_, index);
        auto it = std::begin(commands_);
        std::advance(it, index);
        *it = cmd;

        refresh();
    }

    void addCommand(const app::Commands::Command& cmd)
    {
        const int index = commands_.size();
        beginInsertRows(QModelIndex(), index, index);
        commands_.push_back(cmd);
        endInsertRows(); 
    }

    void moveUp(QModelIndexList& indices)
    {
        app::sortAscending(indices);

        const auto first = indices.first();
        if (first.row() == 0)
            return;

        for (int i=0; i<indices.size(); ++i)
        {
            const auto index = indices[i].row();
            std::swap(commands_[index], commands_[index-1]);
            indices[i] = QAbstractTableModel::index(index -1, 0);
        }
        refresh();
    }
    void moveDown(QModelIndexList& indices)
    {
        app::sortDescending(indices);

        const auto numRows = commands_.size();
        const auto last    = indices.last();
        if (last.row() >= numRows - 1)
            return;

        for (int i=0; i<indices.size(); ++i)
        {
            const auto index = indices[i].row();
            std::swap(commands_[index], commands_[index+1]);
            indices[i] = QAbstractTableModel::index(index + 1, 0);
        }
        refresh();
    }

    void setEnableDisable(const QModelIndexList& list, bool onOff)
    {
        for (const auto& i : list)
        {
            auto& cmd = commands_[i.row()];
            cmd.setEnableCommand(onOff);
        }

        refresh();
    }


    void refresh()
    {
        auto first = QAbstractTableModel::index(0, 0);
        auto last  = QAbstractTableModel::index((int)commands_.size(), (int)Columns::LAST);
        emit dataChanged(first, last);                        
    }
private:
    enum class Columns {
        Exec, Enabled, Comment, /* Args, */ LAST
    };
private:
    friend class CmdSettings;
    app::Commands::CmdList commands_;
};    

CmdSettings::CmdSettings(app::Commands::CmdList cmds) : model_(new Model(cmds))
{
    ui_.setupUi(this);
    ui_.tableCmds->setModel(model_.get());
    ui_.tableCmds->setColumnWidth(1, 32);

    QObject::connect(ui_.tableCmds->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(tableCmds_selectionChanged()));
}

CmdSettings::~CmdSettings()
{}


app::Commands::CmdList CmdSettings::getCommandsCopy() 
{ return model_->commands_; }

void CmdSettings::on_btnAdd_clicked()
{
    app::Commands::Command command;

    DlgCommand dlg(this, command);
    if (dlg.exec() == QDialog::Rejected)
        return;

    model_->addCommand(command);
}

void CmdSettings::on_btnDel_clicked()
{
    const auto indices = ui_.tableCmds->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    model_->delCommand(indices[0].row());

    tableCmds_selectionChanged();
}

void CmdSettings::on_btnEdit_clicked()
{
    const auto indices = ui_.tableCmds->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto row = indices[0].row();

    auto command = model_->getCommand(row);
    DlgCommand dlg(this, command);
    if (dlg.exec() == QDialog::Rejected)
        return;

    model_->setCommand(row, command);
 
}

void CmdSettings::on_btnMoveUp_clicked()
{
    auto indices = ui_.tableCmds->selectionModel()->selectedRows();

    model_->moveUp(indices);

    QItemSelection selection;
    for (auto& i : indices)
        selection.select(i, i);

    auto* model = ui_.tableCmds->selectionModel();
    model->setCurrentIndex(selection.indexes().first(),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    model->select(selection, 
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void CmdSettings::on_btnMoveDown_clicked()
{
    auto indices = ui_.tableCmds->selectionModel()->selectedRows();

    model_->moveDown(indices);

    QItemSelection selection;
    for (auto& i : indices)
        selection.select(i, i);

    auto* model = ui_.tableCmds->selectionModel();
    model->setCurrentIndex(selection.indexes().first(),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    model->select(selection, 
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);    
}

void CmdSettings::on_actionAdd_triggered()
{
    on_btnAdd_clicked();
}

void CmdSettings::on_actionDel_triggered()
{
    on_btnDel_clicked();
}

void CmdSettings::on_actionEdit_triggered()
{
    on_btnEdit_clicked();
}

void CmdSettings::on_actionEnable_triggered()
{
    const auto& indices = ui_.tableCmds->selectionModel()->selectedRows();

    model_->setEnableDisable(indices, true);

    ui_.actionDisable->setEnabled(true);
    ui_.actionEnable->setEnabled(false);

}

void CmdSettings::on_actionDisable_triggered()
{
    const auto& indices = ui_.tableCmds->selectionModel()->selectedRows();

    model_->setEnableDisable(indices, false);

    ui_.actionDisable->setEnabled(false);
    ui_.actionEnable->setEnabled(true);    
}


void CmdSettings::on_tableCmds_customContextMenuRequested(QPoint point)
{
    QMenu menu(ui_.tableCmds);
    menu.addAction(ui_.actionAdd);
    menu.addAction(ui_.actionEdit);
    menu.addSeparator();        
    menu.addAction(ui_.actionEnable);
    menu.addAction(ui_.actionDisable);
    menu.addSeparator();    
    menu.addAction(ui_.actionDel);
    menu.exec(QCursor::pos());
}

void CmdSettings::on_tableCmds_doubleClicked(const QModelIndex& index)
{
    on_btnEdit_clicked();
}

void CmdSettings::tableCmds_selectionChanged()
{
    auto indices = ui_.tableCmds->selectionModel()->selectedRows();
    const bool enable = !indices.isEmpty();

    ui_.btnDel->setEnabled(enable);
    ui_.btnEdit->setEnabled(enable);
    ui_.actionDel->setEnabled(enable);
    ui_.actionEdit->setEnabled(enable);
    ui_.actionEnable->setEnabled(false);    
    ui_.actionDisable->setEnabled(false);

    qSort(indices);
    if (!indices.isEmpty())
    {
        const auto numRows = model_->rowCount(QModelIndex());
        const auto first   = indices[0].row();
        const auto last    = indices[indices.size()-1].row();

        ui_.btnMoveUp->setEnabled(first > 0);
        ui_.btnMoveDown->setEnabled(last < numRows -1);

        const auto& command = model_->getCommand(first);
        const bool enabled  = command.isEnabled();
        ui_.actionEnable->setEnabled(!enabled);
        ui_.actionDisable->setEnabled(enabled);
    }
}


} // gui