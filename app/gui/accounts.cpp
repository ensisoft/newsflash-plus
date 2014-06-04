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

#include <ctime>

#include "accounts.h"
#include "../debug.h"
#include "../format.h"

using app::str;

namespace gui
{

Accounts::Accounts(app::accounts& accounts) : accounts_(accounts)
{
    ui_.setupUi(this);
}

Accounts::~Accounts()
{}

void Accounts::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionServerAdd);
    menu.addAction(ui_.actionServerDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionServerSortGroups);
    menu.addAction(ui_.actionServerDownloadList);
    menu.addAction(ui_.actionGroupAdd);
    menu.addAction(ui_.actionGroupAddByName);
    menu.addSeparator();
    menu.addAction(ui_.actionGroupUpdate);
    menu.addAction(ui_.actionGroupUpdateWO);
    menu.addSeparator();
    menu.addAction(ui_.actionGroupOpen);
    menu.addSeparator();
    menu.addAction(ui_.actionGroupDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionGroupEraseContent);
    menu.addSeparator();
    menu.addAction(ui_.actionGroupPurgeContent);

}

void Accounts::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionServerAdd);
    bar.addAction(ui_.actionServerDelete);
    bar.addSeparator();
    bar.addAction(ui_.actionServerSortGroups);
    bar.addAction(ui_.actionGroupAdd);
    bar.addSeparator();
    bar.addAction(ui_.actionGroupUpdate);
    bar.addAction(ui_.actionGroupOpen);
    bar.addSeparator();
    bar.addAction(ui_.actionGroupDelete);
    bar.addAction(ui_.actionGroupEraseContent);
    bar.addAction(ui_.actionGroupPurgeContent);
}

void Accounts::show_advertisment(bool show)
{
    ui_.lblMovie->setMovie(nullptr);
    ui_.lblMovie->setVisible(false);
    ui_.lblPlead->setVisible(false);
    ui_.lblRegister->setVisible(false);
    movie_.release();

    if (!show) return;

    qsrand(std::time(nullptr));

    QString resource;
    QString campaing;    
    if ((qrand() >> 7) & 1)
    {
        resource = ":/resource/uns-special-2.gif";
        campaing = "https://usenetserver.com/partners/?a_aid=foobar1234&amp;a_bid=dcec941d";
    }
    else
    {
        resource = ":/resource/nh-special.gif";
        campaing = "http://www.newshosting.com/en/index.php?&amp;a_aid=foobar1234&amp;a_bid=2b57ce3a";
    }    
    movie_.reset(new QMovie(resource));

    DEBUG(str("Usenet campaing '_1' '_2'", resource, campaing));
    Q_ASSERT(movie_->isValid());
 
    const auto& pix  = movie_->currentPixmap();
    const auto& size = pix.size();
    movie_->start();
    movie_->setSpeed(200);    

    // NOTE: if the movie doesn't show up the problem might have
    // to do with Qt image plugins!

    ui_.lblMovie->setMinimumSize(size);
    ui_.lblMovie->resize(size);
    ui_.lblMovie->setMovie(movie_.get());
    ui_.lblMovie->setVisible(true);
    ui_.lblMovie->setProperty("url", campaing);    
    ui_.lblPlead->setVisible(true);
    ui_.lblRegister->setVisible(true);
}


sdk::uicomponent::info Accounts::get_info() const
{
    const static sdk::uicomponent::info info {
        "accounts.html", true
    };
    return info;
}

} // gui
