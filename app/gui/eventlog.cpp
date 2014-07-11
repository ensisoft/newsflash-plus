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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QAbstractListModel>
#include <newsflash/warnpop.h>

#include "eventlog.h"
#include "mainwindow.h"

namespace gui
{

eventlog::eventlog(app::eventlog& model) : model_(model)
{
    ui_.setupUi(this);
    ui_.listLog->setModel(model.view());
    ui_.actionClearLog->setEnabled(false);

    model.on_event = std::bind(&eventlog::on_event, this,
        std::placeholders::_1);
}

eventlog::~eventlog()
{}

void eventlog::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionClearLog);
}

void eventlog::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionClearLog);
}

void eventlog::activate(QWidget*)
{
    setWindowIcon(QIcon(":/resource/16x16_ico_png/ico_info.png"));
    g_win->update(this);
}

mainwidget::info eventlog::information() const 
{
    return {"eventlog.html", false};
}

void eventlog::on_event(const app::eventlog::event_t& e)
{
    if (e.type == app::eventlog::event::error)
    {
        setWindowIcon(QIcon(":/resource/16x16_ico_png/ico_error.png"));
        g_win->update(this);
    }
    ui_.actionClearLog->setEnabled(true);
}

void eventlog::on_actionClearLog_triggered()
{
    ui_.actionClearLog->setEnabled(false);
    
    model_.clear();
}

void eventlog::on_listLog_customContextMenuRequested(QPoint pos)
{
    QMenu menu(this);
    menu.addAction(ui_.actionClearLog);
    menu.exec(QCursor::pos());
}

} // gui

