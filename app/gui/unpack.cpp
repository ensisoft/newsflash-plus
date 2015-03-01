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
#include "unpack.h"
#include "../debug.h"
#include "../settings.h"
#include "../archive.h"
#include "../unpacker.h"
#include "../utility.h"

namespace gui 
{

Unpack::Unpack(app::Unpacker& unpacker) : model_(unpacker)
{
    ui_.setupUi(this);
    ui_.unpackList->setModel(unpacker.getUnpackList());
    ui_.unpackData->setModel(unpacker.getUnpackData());
    ui_.progressBar->setVisible(false);
    ui_.progressBar->setMinimum(0);
    ui_.progressBar->setMaximum(100);
    ui_.lblStatus->clear();

    QObject::connect(&model_, SIGNAL(unpackStart(const app::Archive&)),
        this, SLOT(unpackStart(const app::Archive&)));
    QObject::connect(&model_, SIGNAL(unpackReady(const app::Archive&)),
        this, SLOT(unpackReady(const app::Archive&)));
    QObject::connect(&model_, SIGNAL(unpackProgress(const QString&, int)),
        this, SLOT(unpackProgress(const QString&, int)));

    DEBUG("Unpack UI created");
}

Unpack::~Unpack()
{
    DEBUG("Unpack UI deleted");
}

void Unpack::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionUnpack);
    bar.addSeparator();    
    bar.addAction(ui_.actionStop);
}

void Unpack::addActions(QMenu& menu)
{
    // no menu actions.
}

void Unpack::loadState(app::Settings& settings)
{
    app::loadTableLayout("unpack", ui_.unpackList, settings);
    app::loadTableLayout("unpack", ui_.unpackData, settings);


    const auto keepBroken = settings.get("unpack", "keep_broken", true);
    const auto overwrite  = settings.get("unpack", "overwrite_existing_files", false);
    const auto purge      = settings.get("unpack", "purge_on_success", true);

    ui_.chkKeepBroken->setChecked(keepBroken);
    ui_.chkOverwriteExisting->setChecked(keepBroken);
    ui_.chkPurge->setChecked(purge);    
}

void Unpack::saveState(app::Settings& settings)
{
    app::saveTableLayout("unpack", ui_.unpackList, settings);
    app::saveTableLayout("unpack", ui_.unpackData, settings);

    const auto keepBroken = ui_.chkKeepBroken->isChecked();
    const auto overwrite  = ui_.chkOverwriteExisting->isChecked();
    const auto purge      = ui_.chkPurge->isChecked();
    settings.set("unpack", "keep_broken", keepBroken);
    settings.set("unpack", "overwrite_existing_files", overwrite);
    settings.set("unpack", "purge_on_success", purge);

}

void Unpack::shutdown()
{
    model_.stopUnpack();
}

void Unpack::setUnpackEnabled(bool onOff)
{
    model_.setEnabled(onOff);
}

void Unpack::on_actionUnpack_triggered()
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
    model_.addUnpack(arc);
}

void Unpack::on_actionStop_triggered()
{
    model_.stopUnpack();
}

void Unpack::unpackStart(const app::Archive& arc)
{
    ui_.progressBar->setVisible(true);
    ui_.actionStop->setEnabled(true);
    ui_.lblStatus->setVisible(true);
}

void Unpack::unpackReady(const app::Archive& arc)
{
    DEBUG("unpackReady");

    ui_.progressBar->setVisible(false);
    ui_.actionStop->setEnabled(false);
    ui_.lblStatus->setVisible(false);
}

void Unpack::unpackProgress(const QString& target, int done)
{
    DEBUG("unpackProgress %1", done);

    ui_.progressBar->setValue(done);    
    ui_.lblStatus->setText(target);
    ui_.unpackData->scrollToBottom();
}

} // gui
