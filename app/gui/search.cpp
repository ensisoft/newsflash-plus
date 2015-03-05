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

#define LOGTAG "search"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#include <newsflash/warnpop.h>
#include "search.h"
#include "../debug.h"
#include "../search.h"
#include "../searchengine.h"

namespace gui
{

Search::Search(app::SearchEngine& engine) : engine_(engine)
{
    ui_.setupUi(this);
    ui_.btnBasic->setChecked(true);
    ui_.wBasic->setVisible(true);
    ui_.wAdvanced->setVisible(false);
    ui_.wTV->setVisible(false);
    ui_.wMusic->setVisible(false);

    QObject::connect(ui_.btnBasic, SIGNAL(clicked()), 
        ui_.wBasic, SLOT(show()));
    QObject::connect(ui_.btnBasic, SIGNAL(clicked()),
        ui_.wAdvanced, SLOT(hide()));
    QObject::connect(ui_.btnBasic, SIGNAL(clicked()),
        ui_.wMusic, SLOT(hide()));
    QObject::connect(ui_.btnBasic, SIGNAL(clicked()),
        ui_.wTV, SLOT(hide()));

    QObject::connect(ui_.btnAdvanced, SIGNAL(clicked()), 
        ui_.wBasic, SLOT(hide()));
    QObject::connect(ui_.btnAdvanced, SIGNAL(clicked()), 
        ui_.wAdvanced, SLOT(show()));
    QObject::connect(ui_.btnAdvanced, SIGNAL(clicked()),
        ui_.wMusic, SLOT(hide()));
    QObject::connect(ui_.btnAdvanced, SIGNAL(clicked()),
        ui_.wTV, SLOT(hide()));

    QObject::connect(ui_.btnTelevision, SIGNAL(clicked()), 
        ui_.wBasic, SLOT(hide()));
    QObject::connect(ui_.btnTelevision, SIGNAL(clicked()), 
        ui_.wAdvanced, SLOT(hide()));
    QObject::connect(ui_.btnTelevision, SIGNAL(clicked()),
        ui_.wMusic, SLOT(hide()));
    QObject::connect(ui_.btnTelevision, SIGNAL(clicked()),
        ui_.wTV, SLOT(show()));

    QObject::connect(ui_.btnMusic, SIGNAL(clicked()), 
        ui_.wBasic, SLOT(hide()));
    QObject::connect(ui_.btnMusic, SIGNAL(clicked()), 
        ui_.wAdvanced, SLOT(hide()));
    QObject::connect(ui_.btnMusic, SIGNAL(clicked()),
        ui_.wMusic, SLOT(show()));
    QObject::connect(ui_.btnMusic, SIGNAL(clicked()),
        ui_.wTV, SLOT(hide()));    

    DEBUG("Search UI created");
}

Search::~Search()
{
    DEBUG("Search UI deleted");
}

void Search::addActions(QMenu& menu)
{}

void Search::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionRefresh);
    bar.addSeparator();
    bar.addAction(ui_.actionDownload);
    bar.addSeparator();
    bar.addAction(ui_.actionSettings);
    bar.addSeparator();
    bar.addAction(ui_.actionStop);
}

void Search::on_btnSearch_clicked()
{
    
}

} // gui