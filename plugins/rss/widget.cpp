// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#define PLUGIN_IMPL

#define LOGTAG "rss"

#include <newsflash/config.h>

#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/format.h>
#include <newsflash/sdk/eventlog.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#include <newsflash/warnpop.h>

#include "widget.h"

namespace rss
{

widget::widget(sdk::window& win) : win_(win)
{
    ui_.setupUi(this);

    rss_ = dynamic_cast<sdk::rssmodel*>(win.create_model("rss"));

    if (rss_ == nullptr)
        throw std::runtime_error("no rss model available");

    ui_.tableView->setModel(rss_->view());

    DEBUG("rss::widget created");
}

widget::~widget()
{
    DEBUG("rss::widget deleted");
}

void widget::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();    
    menu.addAction(ui_.actionDownload);
    menu.addAction(ui_.actionDownloadTo);
    menu.addSeparator();    
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();        
    menu.addAction(ui_.actionSave);
    menu.addSeparator();        
    menu.addAction(ui_.actionSettings);
    menu.addSeparator();        
    menu.addAction(ui_.actionStop);

}

void widget::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionRefresh);
    bar.addSeparator();
    bar.addAction(ui_.actionDownload);
    bar.addSeparator();    
    bar.addAction(ui_.actionSave);
    bar.addSeparator();    
    bar.addAction(ui_.actionSettings);
    bar.addSeparator();    
    bar.addAction(ui_.actionStop);    
}

void widget::activate(QWidget*)
{

}

void widget::save(sdk::datastore& store) 
{

}

void widget::load(const sdk::datastore& store)
{}

sdk::widget::info widget::information() const
{
    return {"rss.html", true};
}

void widget::on_actionRefresh_triggered()
{
    
}

void widget::on_actionDownload_triggered()
{

}

void widget::on_actionSave_triggered()
{

}

void widget::on_actionSettings_triggered()
{}

void widget::on_actionStop_triggered()
{}


} // rss

int qInitResources_rss();

PLUGIN_API sdk::widget* create_widget(sdk::window* win, int version)
{
    if (version != sdk::widget::version)
        return nullptr;

    qInitResources_rss();
    try
    {
        return new rss::widget(*win);
    }
    catch (const std::exception& e)
    {
        ERROR(sdk::str("rss widget exception _1", e.what()));
    }
    return nullptr;
}

