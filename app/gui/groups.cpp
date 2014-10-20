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

namespace gui
{

groups::groups()
{
    ui_.setupUi(this);
    ui_.tableGroups->setModel(&model_);
    ui_.tableGroups->setColumnWidth(0, 150);
    ui_.progressBar->setVisible(false);
}

groups::~groups()
{}

void groups::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionAdd);
}

void groups::add_actions(QToolBar& bar)
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

mainwidget::info groups::information() const
{
    return {"groups.html", true, true};
}

void groups::on_actionAdd_triggered()
{

}

void groups::on_actionRemove_triggered()
{}

void groups::on_actionOpen_triggered()
{}

void groups::on_actionUpdate_triggered()
{}

void groups::on_actionUpdateOptions_triggered()
{}


void groups::on_actionDelete_triggered()
{}

void groups::on_actionClean_triggered()
{}


void groups::on_actionInfo_triggered()
{}


} // gui