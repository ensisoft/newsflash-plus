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
#include "../filetype.h"

namespace gui
{

DlgCommand::DlgCommand(QWidget* parent, app::Commands::Command& command) : QDialog(parent), command_(command)
{
    ui_.setupUi(this);
    ui_.editExec->setText(command.exec());
    ui_.editExec->setCursorPosition(0);
    ui_.editArgs->setText(command.args());
    ui_.editArgs->setCursorPosition(0);
    ui_.editComment->setText(command.comment());
    ui_.editComment->setCursorPosition(0);

    using w = app::Commands::Command::When;

    const auto when = command.when();
    ui_.chkOnFile->setChecked(when.test(w::OnFileDownload));
    ui_.chkOnPack->setChecked(when.test(w::OnPackDownload));
    ui_.chkOnRepair->setChecked(when.test(w::OnArchiveRepair));
    ui_.chkOnUnpack->setChecked(when.test(w::OnArchiveUnpack));

    ui_.grpEnableCommand->setChecked(command.isEnabled());
    ui_.grpEnableCondition->setChecked(command.isCondEnabled());

    auto beg = app::FileTypeIterator::begin();
    auto end = app::FileTypeIterator::end();
    for (; beg != end; ++beg)
    {
        ui_.cmbRHS->addItem(toString(*beg));
    }
    ui_.cmbRHS->addItem(app::toString(true));
    ui_.cmbRHS->addItem(app::toString(false));

    const auto& cond = command.condition();
    ui_.cmbLHS->setCurrentIndex(
        ui_.cmbLHS->findText(cond.getLHS()));

    ui_.cmbRHS->setCurrentIndex(
        ui_.cmbRHS->findText(cond.getRHS()));

    ui_.cmbOperator->setCurrentIndex(
        ui_.cmbOperator->findText(cond.getOp()));

}

DlgCommand::~DlgCommand()
{}

void DlgCommand::on_btnBrowseExec_clicked()
{
    QString filter;
    #if defined(WINDOWS_OS)
        filter = "Executables (*.exe)";
    #endif

    auto exec = QFileDialog::getOpenFileName(this,
        tr("Select Executable"), QString(), filter);
    if (exec.isEmpty())
        return;

    ui_.editExec->setText(QDir::toNativeSeparators(exec));
    ui_.editExec->setCursorPosition(0);
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

    auto args = ui_.editArgs->text();
    auto comment = ui_.editComment->text();
    bool cmdEnabled = ui_.grpEnableCommand->isChecked();
    bool condEnabled = ui_.grpEnableCondition->isChecked();

    command_.setExec(exec);
    command_.setArgs(args);
    command_.setComment(comment);
    command_.setEnableCommand(cmdEnabled);
    command_.setEnableCondition(condEnabled);

    using w = app::Commands::Command::When;

    app::Commands::Command::WhenFlags flags;
    flags.set(w::OnFileDownload, ui_.chkOnFile->isChecked());
    flags.set(w::OnPackDownload, ui_.chkOnPack->isChecked());
    flags.set(w::OnArchiveRepair, ui_.chkOnRepair->isChecked());
    flags.set(w::OnArchiveUnpack, ui_.chkOnUnpack->isChecked());
    command_.setWhen(flags);

    const auto lhs = ui_.cmbLHS->currentText();
    const auto rhs = ui_.cmbRHS->currentText();
    const auto op  = ui_.cmbOperator->currentText();

    app::Commands::Condition cond(lhs, op, rhs);
    command_.setCondition(cond);

    accept();
}

void DlgCommand::on_btnDefault_clicked()
{}

} // gui
