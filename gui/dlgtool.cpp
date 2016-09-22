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

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QtGui/QKeyEvent>
#  include <QtGui/QFileDialog>
#  include <QDir>
#  include <QFileInfo>
#include "newsflash/warnpop.h"

#include "dlgtool.h"

namespace gui
{

DlgTool::DlgTool(QWidget* parent, app::tools::tool& tool) : QDialog(parent), tool_(tool)
{
    ui_.setupUi(this);
    ui_.editShortcut->installEventFilter(this);

    if (tool.isValid())
    {
        ui_.editApplication->setText(tool.binary());
        ui_.editApplication->setCursorPosition(0);
        ui_.editName->setText(tool.name());
        ui_.editName->setCursorPosition(0);
        ui_.editArgs->setText(tool.arguments());
        ui_.editArgs->setCursorPosition(0);
        ui_.editShortcut->setText(tool.shortcut());
        ui_.editShortcut->setCursorPosition(0);

        using type = app::FileType;

        const auto bits = tool.types();
        ui_.btnAudio->setChecked(bits.test(type::Audio));
        ui_.btnVideo->setChecked(bits.test(type::Video));
        ui_.btnImage->setChecked(bits.test(type::Image));
        ui_.btnText->setChecked(bits.test(type::Text));
        ui_.btnArchive->setChecked(bits.test(type::Archive));
        ui_.btnParity->setChecked(bits.test(type::Parity));
        ui_.btnDocument->setChecked(bits.test(type::Document));
        ui_.btnOther->setChecked(bits.test(type::Other));
    }
}

DlgTool::~DlgTool()
{}

void DlgTool::on_btnBrowse_clicked()
{
    QString filter;

    #if defined(WINDOWS_OS)
        filter = "Executables (*.exe)";
    #endif

    QString executable = QFileDialog::getOpenFileName(this,
        tr("Select Application"), QString(), filter);

    if (executable.isEmpty())
        return;

    QFileInfo info(executable);

    ui_.editApplication->setText(QDir::toNativeSeparators(executable));
    ui_.editApplication->setCursorPosition(0);
    ui_.editName->setText(info.baseName());
    ui_.editName->setCursorPosition(0);
}


void DlgTool::on_btnAccept_clicked()
{
    const QString& binary = ui_.editApplication->text();
    if (binary.isEmpty())
    {
        ui_.editApplication->setFocus();
        return;
    }

    const QString& name = ui_.editName->text();
    if (name.isEmpty())
    {
        ui_.editName->setFocus();
        return;
    }

    const QString& args = ui_.editArgs->text();
    if (args.isEmpty())
    {
        ui_.editArgs->setFocus();
        return;
    }

    const QString& shortcut = ui_.editShortcut->text();

    using type = app::FileType;

    app::tools::bitflag bits;
    if (ui_.btnAudio->isChecked())
        bits.set(type::Audio);
    if (ui_.btnVideo->isChecked())
        bits.set(type::Video);
    if (ui_.btnImage->isChecked())
        bits.set(type::Image);
    if (ui_.btnText->isChecked())
        bits.set(type::Text);
    if (ui_.btnArchive->isChecked())
        bits.set(type::Archive);
    if (ui_.btnParity->isChecked())
        bits.set(type::Parity);
    if (ui_.btnOther->isChecked())
        bits.set(type::Other);
    if (ui_.btnDocument->isChecked())
        bits.set(type::Document);

    tool_.setBinary(binary);
    tool_.setName(name);
    tool_.setArguments(args);
    tool_.setShortcut(shortcut);
    tool_.setTypes(bits);

    accept();
}

void DlgTool::on_btnCancel_clicked()
{
    reject();
}

void DlgTool::on_btnDefault_clicked()
{
    ui_.editArgs->setText("${file}");
}

void DlgTool::on_btnReset_clicked()
{
    ui_.editShortcut->clear();
}

bool DlgTool::eventFilter(QObject* receiver, QEvent* event)
{
    if (event->type() != QEvent::KeyPress)
        return QDialog::eventFilter(receiver, event);

    QKeyEvent* key = static_cast<QKeyEvent*>(event);
    if (key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab || key->key() == Qt::Key_Escape)
        return false;

    if (key->modifiers() == Qt::NoModifier)
        return false;

    // todo: this code is probably pretty broken.
    // figure out a real way of mapping input key events into key seqeunces

    QString shortcut;    
    if (key->modifiers() & Qt::ShiftModifier)
        shortcut += "Shift+";
    if (key->modifiers() & Qt::ControlModifier)
        shortcut += "Ctrl+";
    if (key->modifiers() & Qt::AltModifier)
        shortcut += "Alt+";

    QChar c(key->key());
    if (c.isPrint() && c != Qt::Key_Space)
    {
        shortcut += c.toUpper();
        ui_.editShortcut->setText(shortcut);
    }
    return true;

}


} // gui

