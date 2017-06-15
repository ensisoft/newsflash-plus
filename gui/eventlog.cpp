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

#define LOGTAG "eventlog"

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#include "newsflash/warnpop.h"
#include "eventlog.h"
#include "app/eventlog.h"
#include "app/debug.h"
#include "app/settings.h"
#include "app/utility.h"

namespace gui
{

EventLog::EventLog() : numEvents_(0)
{
    auto& model = app::EventLog::get();

    ui_.setupUi(this);
    ui_.listLog->setModel(&model);
    ui_.actionClearLog->setEnabled(false);

    QObject::connect(&model, SIGNAL(newEvent(const app::Event&)),
        this, SLOT(newEvent(const app::Event&)));

    DEBUG("EventLog UI created");
}

EventLog::~EventLog()
{
    DEBUG("EventLog UI deleted");
}

void EventLog::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionClearLog);
}

void EventLog::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionClearLog);
}

void EventLog::loadState(app::Settings& settings)
{
    app::loadState("eventlog", ui_.chkEng, settings);
    app::loadState("eventlog", ui_.chkApp, settings);
    app::loadState("eventlog", ui_.chkGui, settings);
    app::loadState("eventlog", ui_.chkOther, settings);
    app::loadState("eventlog", ui_.chkInfo, settings);
    app::loadState("eventlog", ui_.chkWarn, settings);
    app::loadState("eventlog", ui_.chkError, settings);
}

void EventLog::saveState(app::Settings& settings)
{
    app::saveState("eventlog", ui_.chkEng, settings);
    app::saveState("eventlog", ui_.chkApp, settings);
    app::saveState("eventlog", ui_.chkGui, settings);
    app::saveState("eventlog", ui_.chkOther, settings);
    app::saveState("eventlog", ui_.chkInfo, settings);
    app::saveState("eventlog", ui_.chkWarn, settings);
    app::saveState("eventlog", ui_.chkError, settings);
}

void EventLog::activate(QWidget*)
{
    numEvents_ = 0;
    setWindowIcon(QIcon("icons:ico_info.png"));
    setWindowTitle("Log");
}

void EventLog::startup()
{
    filter();
}

MainWidget::info EventLog::getInformation() const 
{
    return {"eventlog.html", true};
}

void EventLog::on_actionClearLog_triggered()
{
    auto& model = app::EventLog::get();

    ui_.actionClearLog->setEnabled(false);
    
    model.clear();
}

void EventLog::on_listLog_customContextMenuRequested(QPoint pos)
{
    QMenu menu(this);
    menu.addAction(ui_.actionClearLog);
    menu.exec(QCursor::pos());
}

void EventLog::on_chkEng_clicked()
{
    filter();
}

void EventLog::on_chkApp_clicked()
{
    filter();
}

void EventLog::on_chkGui_clicked()
{
    filter();
}

void EventLog::on_chkOther_clicked()
{
    filter();
}

void EventLog::on_chkInfo_clicked()
{
    filter();
}

void EventLog::on_chkWarn_clicked()
{
    filter();
}

void EventLog::on_chkError_clicked()
{
    filter();
}

void EventLog::newEvent(const app::Event& event)
{
    if (event.type == app::Event::Type::Note)
        return;

    numEvents_++;
    if (event.type == app::Event::Type::Error)
    {
        setWindowIcon(QIcon("icons:ico_error.png"));
        setWindowTitle(QString("Log (%1)").arg(numEvents_));
    }
    if (event.type == app::Event::Type::Warning)
    {
        setWindowIcon(QIcon("icons:ico_warning.png"));
        setWindowTitle(QString("Log (%1)").arg(numEvents_));
    }

    ui_.actionClearLog->setEnabled(true);

}

void EventLog::filter()
{
    app::EventLog::Filtering filtering;
    filtering.ShowModuleEng   = ui_.chkEng->isChecked();
    filtering.ShowModuleApp   = ui_.chkApp->isChecked();
    filtering.ShowModuleGui   = ui_.chkGui->isChecked();
    filtering.ShowModuleOther = ui_.chkOther->isChecked();
    filtering.ShowInfos       = ui_.chkInfo->isChecked();
    filtering.ShowWarnings    = ui_.chkWarn->isChecked();
    filtering.ShowErrors      = ui_.chkError->isChecked();
    
    auto& model = app::EventLog::get();
    model.filter(filtering);
}

} // gui

