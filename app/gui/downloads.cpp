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

#define LOGTAG "gui"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#  include <QtGui/QSplitter>
#  include <QtGui/QMouseEvent>
#include <newsflash/warnpop.h>

#include "downloads.h"
#include "../debug.h"
#include "../format.h"
#include "../eventlog.h"
#include "../settings.h"

namespace gui
{

downloads::downloads() : panels_y_pos_(0)
{
    ui_.setupUi(this);
    ui_.tableConns->setModel(&conns_);
    ui_.tableTasks->setModel(&tasks_);

    // auto* splitter = new QSplitter(this);
    // splitter->setOrientation(Qt::Vertical);
    // splitter->addWidget(ui_.grpTasks);
    // splitter->addWidget(ui_.grpConns);    

    ui_.splitter->installEventFilter(this);

    DEBUG("Created downloads UI");
}


downloads::~downloads()
{
    DEBUG("Destroyed downloads UI");
}

void downloads::add_actions(QMenu& menu)
{}

void downloads::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionAutoConnect);
    bar.addAction(ui_.actionPreferSSL);
    bar.addAction(ui_.actionThrottle);
    bar.addSeparator();
    bar.addAction(ui_.actionTaskPause);
    bar.addAction(ui_.actionTaskResume);
    bar.addAction(ui_.actionTaskMoveUp);
    bar.addAction(ui_.actionTaskMoveDown);
    bar.addAction(ui_.actionTaskMoveTop);
    bar.addAction(ui_.actionTaskMoveBottom);
    bar.addSeparator();    
    bar.addAction(ui_.actionTaskDelete);
    bar.addAction(ui_.actionTaskClear);    
    bar.addSeparator();        

}

// void downloads::load(const app::datastore& data) 
// {
//     const auto task_list_height = data.get("downloads", "task_list_height", 0);
//     if (task_list_height)
//         ui_.grpConns->setFixedHeight(task_list_height);

//     ui_.actionThrottle->setChecked(data.get("downloads", "throttle", false));
//     ui_.actionAutoConnect->setChecked(data.get("downloads", "connect", false));
//     ui_.actionPreferSSL->setChecked(data.get("downloads", "ssl", true));
//     //ui_.action
// }

// void downloads::save(app::datastore& data)
// {
//     data.set("downloads", "task_list_height", ui_.grpConns->height());
//     data.set("downloads", "throttle", ui_.actionThrottle->isChecked());
//     data.set("downloads", "connect", ui_.actionAutoConnect->isChecked());
//     data.set("downloads", "ssl", ui_.actionPreferSSL->isChecked());
//     data.set("downloads", "remove_complete", ui_.chkRemoveComplete->isChecked());
//     data.set("downloads", "group_related", ui_.chkGroupSimilar->isChecked());
// }

bool downloads::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        const auto* mickey = static_cast<QMouseEvent*>(event);
        const auto& point  = mapFromGlobal(mickey->globalPos());
        const auto  ypos   = point.y();
        if (panels_y_pos_)
        {
            auto change = ypos - panels_y_pos_;
            auto height = ui_.grpTasks->height();
            if (height + change < 150)
                return true;

            height = ui_.grpConns->height();
            if (height - change < 100)
                return true;

            ui_.grpConns->setFixedHeight(height - change);
        }
        panels_y_pos_ = ypos;
        return true;
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        panels_y_pos_ = 0;
        return true;
    }

    return QObject::eventFilter(obj, event);
}

} // gui
