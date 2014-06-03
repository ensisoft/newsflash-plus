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

namespace {
    class model : public QAbstractListModel
    {
    public:
        model(QObject* parent, const app::eventlog& events) : QAbstractListModel(parent), 
            events_(events)
        {}

       ~model()
        {}

        int rowCount(const QModelIndex&) const
        {
            return static_cast<int>(events_.size());
        }
        QVariant data(const QModelIndex& index, int role) const
        {
            const auto& event = events_[index.row()];
            switch (role)
            {
                case Qt::DecorationRole:
                switch (event.type) {
                    case app::eventlog::event_t::warning:
                        return QIcon(":/resource/16x16_ico_png/ico_warning.png");
                    case app::eventlog::event_t::info:
                       return QIcon(":/resource/16x16_ico_png/ico_info.png");
                    case app::eventlog::event_t::error:
                       return QIcon(":/resource/16x16_ico_png/ico_error.png");
                    default:
                       Q_ASSERT("missing event type");
                       break;
                }
                break;

                case Qt::DisplayRole:
                    return QString("[%1] [%2] %3")
                      .arg(event.time.toString("hh:mm:ss:zzz"))
                      .arg(event.context)
                      .arg(event.message);                
                    break;
            }
            return QVariant();
        }

    private:
        const app::eventlog& events_;
    };
} // namespace

namespace gui
{

Eventlog::Eventlog(app::eventlog& events) : events_(events)
{
    ui_.setupUi(this);
    ui_.listLog->setModel(new model(this, events));
    ui_.actionClearLog->setEnabled(false);
}

Eventlog::~Eventlog()
{}

void Eventlog::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionClearLog);
}

void Eventlog::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionClearLog);
}

void Eventlog::on_actionClearLog_triggered()
{
    events_.clear();
}

void Eventlog::on_listLog_customContextMenuRequested(QPoint pos)
{
    QMenu menu(this);
    menu.addAction(ui_.actionClearLog);
    menu.exec(QCursor::pos());
}

sdk::uicomponent::info Eventlog::get_info() const 
{
    const static sdk::uicomponent::info info {
        "eventlog.html", false
    };
    return info;
}

} // gui

