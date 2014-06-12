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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#  include <QtGui/QPixmap>
#  include <QtGui/QMovie>
#include <newsflash/warnpop.h>

#include "groups.h"

#include "../groups.h"

namespace {
    app::groups& get_groups_model(sdk::model& model)
    {
        return static_cast<app::groups&>(model);
    }

} // namespace 

namespace gui
{

Groups::Groups(sdk::model& model) : model_(model)
{
    ui_.setupUi(this);
    ui_.tableGroups->setModel(model.view());
    ui_.tableGroups->setColumnWidth(0, 150);
    ui_.progressBar->setVisible(false);
}

Groups::~Groups()
{}

void Groups::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionAdd);
}

void Groups::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionAdd);
    bar.addAction(ui_.actionRemove);
    bar.addSeparator();
    bar.addAction(ui_.actionOpen);
    bar.addSeparator();
    bar.addAction(ui_.actionUpdate);
    bar.addAction(ui_.actionUpdateOptions);
    bar.addSeparator();
    bar.addAction(ui_.actionDelete);
    bar.addAction(ui_.actionClean);
    bar.addSeparator();
    bar.addAction(ui_.actionInfo);
}

sdk::widget::info Groups::information() const
{
    return {"groups.html", true};
}

void Groups::on_actionAdd_triggered()
{

}

void Groups::on_actionRemove_triggered()
{}

void Groups::on_actionOpen_triggered()
{}

void Groups::on_actionUpdate_triggered()
{}

void Groups::on_actionUpdateOptions_triggered()
{}


void Groups::on_actionDelete_triggered()
{}

void Groups::on_actionClean_triggered()
{}


void Groups::on_actionInfo_triggered()
{}


} // gui