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
#  include <QtGui/QFileDialog>
#  include <QDir>
#  include <QFileInfo>
#include <newsflash/warnpop.h>
#include "dlgcommand.h"

namespace gui
{

DlgCommand::DlgCommand(QWidget* parent, app::Commands::Command& command) : QDialog(parent), command_(command)
{
    ui_.setupUi(this);
    ui_.editExec->setText(command.exec());
    ui_.editExec->setCursorPosition(0);
    ui_.editFile->setText(command.file());
    ui_.editFile->setCursorPosition(0);
    ui_.editName->setText(command.name());
    ui_.editName->setCursorPosition(0);
    ui_.editArgs->setText(command.args());
    ui_.editArgs->setCursorPosition(0);
    ui_.editComment->setText(command.comment());
    ui_.editComment->setCursorPosition(0);

    ui_.grpEnable->setChecked(command.isEnabled());
}

DlgCommand::~DlgCommand()
{}

void DlgCommand::on_btnBrowseExec_clicked()
{
    QString filter;
    #if defined(WINDOWS_OS)
        filter = "*.exe";
    #endif

    auto exec = QFileDialog::getOpenFileName(this,
        tr("Select Executable"), QString(), filter);
    if (exec.isEmpty())
        return;

    ui_.editExec->setText(QDir::toNativeSeparators(exec));
    ui_.editExec->setCursorPosition(0);
}

void DlgCommand::on_btnBrowseFile_clicked()
{
    auto file = QFileDialog::getOpenFileName(this,
        tr("Select Script File"));
    if (file.isEmpty())
        return;

    QFileInfo info(file);
    ui_.editFile->setText(QDir::toNativeSeparators(file));
    ui_.editFile->setCursorPosition(0);    
    ui_.editName->setText(info.completeBaseName());
    ui_.editName->setCursorPosition(0);    
}

void DlgCommand::on_btnCancel_clicked()
{
    reject();
}

void DlgCommand::on_btnAccept_clicked()
{
    auto exec = ui_.editExec->text();
    if (exec.isEmpty())
    {
        ui_.editExec->setFocus();
        return;
    }
    auto file = ui_.editFile->text();
    auto name = ui_.editName->text();
    auto args = ui_.editArgs->text();
    auto comment = ui_.editComment->text();
    bool enabled = ui_.grpEnable->isChecked();

    command_.setExec(exec);
    command_.setName(name);
    command_.setArgs(args);
    command_.setFile(file);
    command_.setComment(comment);
    command_.setEnabled(enabled);

    accept();
}

void DlgCommand::on_btnDefault_clicked()
{}

} // gui
