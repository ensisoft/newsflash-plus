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
#include "repair.h"
#include "unpack.h"
#include "../debug.h"
#include "../settings.h"

namespace gui 
{

Archives::Archives(Unpack& unpack, Repair& repair) : unpack_(unpack), repair_(repair)
{
    ui_.setupUi(this);

    ui_.tabWidget->blockSignals(true);
    {
        const auto text = repair.windowTitle();
        const auto icon = repair.windowIcon();        
        ui_.tabWidget->addTab(&repair, icon, text);
        current_ = &repair_;
    }
    {
        const auto text = unpack.windowTitle();
        const auto icon = unpack.windowIcon();     
        ui_.tabWidget->addTab(&unpack, icon, text);
    }

    ui_.tabWidget->setCurrentIndex(0);
    ui_.tabWidget->blockSignals(false);
    ui_.actionRepair->setChecked(true);
    ui_.actionUnpack->setChecked(true);

    DEBUG("Archives UI created");
}

Archives::~Archives()
{
    DEBUG("Archives UI deleted");
}

void Archives::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionRepair);
    bar.addAction(ui_.actionUnpack);
    bar.addSeparator();

    auto* w = getCurrent();
    w->addActions(bar);
}

void Archives::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionRepair);
    menu.addAction(ui_.actionUnpack);
    menu.addSeparator();

    auto* w = getCurrent();
    w->addActions(menu);
}

void Archives::activate(QWidget* parent)
{}

void Archives::deactivate()
{}

void Archives::loadState(app::Settings& settings)
{
    unpack_.loadState(settings);
    repair_.loadState(settings);

    bool unpack = ui_.actionUnpack->isChecked();
    bool repair = ui_.actionRepair->isChecked();
    unpack = settings.get("archives", "unpack", unpack);
    repair = settings.get("archives", "repair", repair);
    ui_.actionUnpack->setChecked(unpack);
    ui_.actionRepair->setChecked(repair);

    unpack_.setUnpackEnabled(unpack);
    repair_.setRepairEnabled(repair);
}

void Archives::saveState(app::Settings& settings)
{
    unpack_.saveState(settings);
    repair_.saveState(settings);

    const auto unpack = ui_.actionUnpack->isChecked();
    const auto repair = ui_.actionRepair->isChecked();
    settings.set("archives", "unpack", unpack);
    settings.set("archives", "repair", repair);
}

void Archives::shutdown()
{
    ui_.tabWidget->blockSignals(true);
    ui_.tabWidget->clear();
    unpack_.shutdown();
    repair_.shutdown();

    repair_.setParent(nullptr);
    unpack_.setParent(nullptr);
}

void Archives::refresh(bool isActive)
{
    unpack_.refresh(current_ == &unpack_);
    repair_.refresh(current_ == &repair_);
}

void Archives::on_tabWidget_currentChanged(int index)
{
    Q_ASSERT(current_);

    auto* w = ui_.tabWidget->currentWidget();
    auto* m = static_cast<MainWidget*>(w);

    current_->deactivate();
    current_ = m;
    current_->activate(this);

    emit updateMenu(this);
}

void Archives::on_actionRepair_triggered()
{
    const auto onOff = ui_.actionRepair->isChecked();
    repair_.setRepairEnabled(onOff);
}

void Archives::on_actionUnpack_triggered()
{
    const auto onOff = ui_.actionUnpack->isChecked();
    unpack_.setUnpackEnabled(onOff);
}

MainWidget* Archives::getCurrent()
{
    auto* p = ui_.tabWidget->currentWidget();
    return static_cast<MainWidget*>(p);
}


} // gui
