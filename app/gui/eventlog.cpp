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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#include <newsflash/warnpop.h>
#include "eventlog.h"
#include "../eventlog.h"
#include "../debug.h"

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

void EventLog::activate(QWidget*)
{
    numEvents_ = 0;
    setWindowIcon(QIcon("icons:ico_info.png"));
    setWindowTitle("Log");
}

MainWidget::info EventLog::getInformation() const 
{
    return {"eventlog.html", false};
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

} // gui

