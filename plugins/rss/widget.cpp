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
#  include <QtGui/QMessageBox>
#include <newsflash/warnpop.h>

#include "widget.h"

namespace rss
{

settings::settings(sdk::bitflag_t& feeds) : feeds_(feeds)
{
    ui_.setupUi(this);

    using namespace sdk;

    ui_.chkTVForeign->setChecked(feeds & category::tv_int);
    ui_.chkTVHD->setChecked(feeds & category::tv_hd);
    ui_.chkTVSD->setChecked(feeds & category::tv_sd);
    ui_.chkComputerPC->setChecked(feeds & category::apps_pc);
    ui_.chkComputerISO->setChecked(feeds & category::apps_iso);
    ui_.chkComputerMac->setChecked(feeds & category::apps_mac);
    ui_.chkComputerAndroid->setChecked(feeds & category::apps_android);
    ui_.chkMusicMP3->setChecked(feeds & category::audio_mp3);
    ui_.chkMusicLosless->setChecked(feeds & category::audio_lossless);
    ui_.chkMusicVideo->setChecked(feeds & category::audio_video);
    ui_.chkMoviesForeign->setChecked(feeds & category::movies_int);
    ui_.chkMoviesSD->setChecked(feeds & category::movies_sd);
    ui_.chkMoviesHD->setChecked(feeds & category::movies_hd);
    ui_.chkConsoleNintendo->setChecked(feeds & category::console_nintendo);
    ui_.chkConsolePlaystation->setChecked(feeds & category::console_playstation);
    ui_.chkConsoleXbox->setChecked(feeds & category::console_microsoft);
    ui_.chkXXXDVD->setCheckable(feeds & category::xxx_dvd);
    ui_.chkXXXHD->setChecked(feeds & category::xxx_hd);
    ui_.chkXXXSD->setChecked(feeds & category::xxx_sd);
}

settings::~settings()
{}

void settings::accept() 
{
#define b(x) bitflag_t(x)

    feeds_ = 0;

    using namespace sdk;
    if (ui_.chkTVForeign->isChecked())
        feeds_ |= b(category::tv_int);
    if (ui_.chkTVHD->isChecked())
        feeds_ |= b(category::tv_hd);
    if (ui_.chkTVSD->isChecked())
        feeds_ |= b(category::tv_sd);
    if (ui_.chkComputerPC->isChecked())
        feeds_ |= b(category::apps_pc);
    if (ui_.chkComputerISO->isChecked())
        feeds_ |= b(category::apps_iso);
    if (ui_.chkComputerMac->isChecked())
        feeds_ |= b(category::apps_mac);
    if (ui_.chkComputerAndroid->isChecked())
        feeds_ |= b(category::apps_android);
    if (ui_.chkMusicMP3->isChecked())
        feeds_ |= b(category::audio_mp3);
    if (ui_.chkMusicLosless->isChecked())
        feeds_ |= b(category::audio_lossless);
    if (ui_.chkMusicVideo->isChecked())
        feeds_ |= b(category::audio_video);
    if (ui_.chkMoviesForeign->isChecked())
        feeds_ |= b(category::movies_int);
    if (ui_.chkMoviesHD->isChecked())
        feeds_ |= b(category::movies_hd);
    if (ui_.chkMoviesSD->isChecked())
        feeds_ |= b(category::movies_sd);
    if (ui_.chkConsoleNintendo->isChecked())
        feeds_ |= b(category::console_nintendo);
    if (ui_.chkConsolePlaystation->isChecked())
        feeds_ |= b(category::console_playstation);
    if (ui_.chkConsoleXbox->isChecked())
        feeds_ |= b(category::console_microsoft);
    if (ui_.chkXXXDVD->isChecked())
        feeds_ |= b(category::xxx_dvd);
    if (ui_.chkXXXHD->isChecked())
        feeds_ |= b(category::xxx_hd);
    if (ui_.chkXXXSD->isChecked())
        feeds_ |= b(category::xxx_sd);
}

widget::widget(sdk::window& win) : feeds_(0), win_(win)
{
    ui_.setupUi(this);

    rss_ = dynamic_cast<sdk::rssmodel*>(win.create_model("rss"));

    if (rss_ == nullptr)
        throw std::runtime_error("no rss model available");

    QObject::connect(rss_, SIGNAL(ready()), this, SLOT(ready()));

    ui_.tableView->setModel(rss_->view());

    ui_.actionDownload->setEnabled(false);
    ui_.actionDownloadTo->setEnabled(false);
    ui_.actionSave->setEnabled(false);
    ui_.actionStop->setEnabled(false);
    ui_.progressBar->setVisible(false);
    ui_.progressBar->setValue(0);
    ui_.progressBar->setRange(0, 0);

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
    store.set("rss", "music", ui_.chkMusic->isChecked());
    store.set("rss", "movies", ui_.chkMovies->isChecked());
    store.set("rss", "tv", ui_.chkTV->isChecked());
    store.set("rss", "console", ui_.chkConsole->isChecked());
    store.set("rss", "computer", ui_.chkComputer->isChecked());
    store.set("rss", "xxx", ui_.chkXXX->isChecked());
    store.set("rss", "feeds", feeds_);
    store.set("rss", "feeds", qlonglong(feeds_));
}

void widget::load(const sdk::datastore& store)
{
    ui_.chkMusic->setChecked(store.get("rss", "music", false));
    ui_.chkMovies->setChecked(store.get("rss", "movies", false));
    ui_.chkTV->setChecked(store.get("rss", "tv", false));
    ui_.chkConsole->setChecked(store.get("rss", "console", false));
    ui_.chkComputer->setChecked(store.get("rss", "computer", false));
    ui_.chkXXX->setChecked(store.get("rss", "xxx", false));

    feeds_ = (sdk::bitflag_t)store.get("rss", "feeds", qlonglong(sdk::category::all));

}

sdk::widget::info widget::information() const
{
    return {"rss.html", true};
}

sdk::settings* widget::settings()
{
    return new rss::settings(feeds_);
}

void widget::on_actionRefresh_triggered()
{
    struct feed {
        QCheckBox* chk;
        sdk::bitflag_t mask;
    } selected_feeds[] = {
        {ui_.chkMusic, BITFLAG(sdk::category::audio)},
        {ui_.chkMovies, BITFLAG(sdk::category::movies)},
        {ui_.chkTV, BITFLAG(sdk::category::tv)},
        {ui_.chkConsole, BITFLAG(sdk::category::console)},
        {ui_.chkComputer, BITFLAG(sdk::category::apps)},
        {ui_.chkXXX, BITFLAG(sdk::category::xxx)}
    };

    bool have_feeds = false;
    bool have_selections = false;

    for (const auto& feed : selected_feeds)
    {
        if (!feed.chk->isChecked())
            continue;

        auto beg = sdk::category_iterator::begin();
        auto end = sdk::category_iterator::end();
        for (; beg != end; ++beg)
        {
            const auto cat = *beg;
            if (!(cat & feed.mask))
                continue;
            if (!(cat & feeds_))
                continue;
            if (rss_->refresh(cat))
                have_feeds = true;

            have_selections = true;            
        }
    }

    if (!have_selections)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Information);
        msg.setText("You haven't selected any feed categories.\r\n" 
                    "Select the sub-categories in RSS settings and main categories in the RSS main window");
        msg.exec();
        return; 
    }
    else if (!have_feeds)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Information);
        msg.setText("There are no feeds available matching the selected categories");
        msg.exec();
        return;
    }

    rss_->clear();

    ui_.progressBar->setVisible(true);
    ui_.actionDownload->setEnabled(false);
    ui_.actionDownloadTo->setEnabled(false);
    ui_.actionStop->setEnabled(true);
}

void widget::on_actionDownload_triggered()
{
    DEBUG("download");
}

void widget::on_actionSave_triggered()
{
    DEBUG("save");
}

void widget::on_actionSettings_triggered()
{
    win_.show_setting("RSS");
}

void widget::on_actionStop_triggered()
{
    DEBUG("stop");
}

void widget::ready()
{
    ui_.progressBar->hide();
    ui_.actionStop->setEnabled(false);
}


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

