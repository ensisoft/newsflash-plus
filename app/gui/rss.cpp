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
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QMessageBox>
#include <newsflash/warnpop.h>

#include "rss.h"
#include "mainwindow.h"
#include "../debug.h"
#include "../format.h"
#include "../eventlog.h"
#include "../datastore.h"

namespace gui
{

rss_feeds_settings_page::rss_feeds_settings_page(app::bitflag_t& feeds) : feeds_(feeds)
{
    ui_.setupUi(this);

    using namespace app;

    ui_.chkTVForeign->setChecked(feeds & media::tv_int);
    ui_.chkTVHD->setChecked(feeds & media::tv_hd);
    ui_.chkTVSD->setChecked(feeds & media::tv_sd);
    ui_.chkComputerPC->setChecked(feeds & media::apps_pc);
    ui_.chkComputerISO->setChecked(feeds & media::apps_iso);
    ui_.chkComputerMac->setChecked(feeds & media::apps_mac);
    ui_.chkComputerAndroid->setChecked(feeds & media::apps_android);
    ui_.chkMusicMP3->setChecked(feeds & media::audio_mp3);
    ui_.chkMusicLosless->setChecked(feeds & media::audio_lossless);
    ui_.chkMusicVideo->setChecked(feeds & media::audio_video);
    ui_.chkMoviesForeign->setChecked(feeds & media::movies_int);
    ui_.chkMoviesSD->setChecked(feeds & media::movies_sd);
    ui_.chkMoviesHD->setChecked(feeds & media::movies_hd);
    ui_.chkConsoleNintendo->setChecked(feeds & media::console_nintendo);
    ui_.chkConsolePlaystation->setChecked(feeds & media::console_playstation);
    ui_.chkConsoleXbox->setChecked(feeds & media::console_microsoft);
    ui_.chkXXXDVD->setCheckable(feeds & media::xxx_dvd);
    ui_.chkXXXHD->setChecked(feeds & media::xxx_hd);
    ui_.chkXXXSD->setChecked(feeds & media::xxx_sd);
}

rss_feeds_settings_page::~rss_feeds_settings_page()
{}

void rss_feeds_settings_page::accept() 
{
    feeds_ = 0;

    using namespace app;
    if (ui_.chkTVForeign->isChecked())
        feeds_ |= BITFLAG(media::tv_int);
    if (ui_.chkTVHD->isChecked())
        feeds_ |= BITFLAG(media::tv_hd);
    if (ui_.chkTVSD->isChecked())
        feeds_ |= BITFLAG(media::tv_sd);
    if (ui_.chkComputerPC->isChecked())
        feeds_ |= BITFLAG(media::apps_pc);
    if (ui_.chkComputerISO->isChecked())
        feeds_ |= BITFLAG(media::apps_iso);
    if (ui_.chkComputerMac->isChecked())
        feeds_ |= BITFLAG(media::apps_mac);
    if (ui_.chkComputerAndroid->isChecked())
        feeds_ |= BITFLAG(media::apps_android);
    if (ui_.chkMusicMP3->isChecked())
        feeds_ |= BITFLAG(media::audio_mp3);
    if (ui_.chkMusicLosless->isChecked())
        feeds_ |= BITFLAG(media::audio_lossless);
    if (ui_.chkMusicVideo->isChecked())
        feeds_ |= BITFLAG(media::audio_video);
    if (ui_.chkMoviesForeign->isChecked())
        feeds_ |= BITFLAG(media::movies_int);
    if (ui_.chkMoviesHD->isChecked())
        feeds_ |= BITFLAG(media::movies_hd);
    if (ui_.chkMoviesSD->isChecked())
        feeds_ |= BITFLAG(media::movies_sd);
    if (ui_.chkConsoleNintendo->isChecked())
        feeds_ |= BITFLAG(media::console_nintendo);
    if (ui_.chkConsolePlaystation->isChecked())
        feeds_ |= BITFLAG(media::console_playstation);
    if (ui_.chkConsoleXbox->isChecked())
        feeds_ |= BITFLAG(media::console_microsoft);
    if (ui_.chkXXXDVD->isChecked())
        feeds_ |= BITFLAG(media::xxx_dvd);
    if (ui_.chkXXXHD->isChecked())
        feeds_ |= BITFLAG(media::xxx_hd);
    if (ui_.chkXXXSD->isChecked())
        feeds_ |= BITFLAG(media::xxx_sd);
}

rss_nzbs_settings_page::rss_nzbs_settings_page(rss_nzbs_settings& data) : data_(data)
{
    ui_.setupUi(this);

    ui_.editUserID->setText(data.userid);
    ui_.editAPIKey->setText(data.apikey);
    //ui_.feedSize->setValue(data.feedsize);
    ui_.grpEnable->setChecked(data.enabled);
}

rss_nzbs_settings_page::~rss_nzbs_settings_page()
{}

bool rss_nzbs_settings_page::validate() const
{
    if (!ui_.grpEnable->isChecked())
        return true;

    const auto& userid = ui_.editUserID->text();
    if (userid.isEmpty())
    {
        ui_.editUserID->setFocus();
        return false;
    }

    const auto& apikey = ui_.editAPIKey->text();
    if (apikey.isEmpty())
    {
        ui_.editAPIKey->setFocus();
        return false;
    }

    return true;
}

void rss_nzbs_settings_page::accept()
{
    data_.userid = ui_.editUserID->text();
    data_.apikey = ui_.editAPIKey->text();
    //data_.feedsize = ui_.feedSize->value();
    data_.enabled = ui_.grpEnable->isChecked();
}

rss::rss(mainwindow& win, app::rss& model) : win_(win), model_(model), feeds_(0)
{
    ui_.setupUi(this);

    ui_.tableView->setModel(model_.view());
    ui_.actionDownload->setEnabled(false);
    ui_.actionDownloadTo->setEnabled(false);
    ui_.actionSave->setEnabled(false);
    ui_.actionOpen->setEnabled(false);
    ui_.actionStop->setEnabled(false);
    ui_.progressBar->setVisible(false);
    ui_.progressBar->setValue(0);
    ui_.progressBar->setRange(0, 0);

    auto* selection = ui_.tableView->selectionModel();
    QObject::connect(selection, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(rowChanged()));

    //QObject::connect(&model_, SIGNAL(ready()), this, SLOT(ready()));
    model.on_ready = std::bind(&rss::ready, this);

    DEBUG("rss::rss created");
}

rss::~rss()
{
    DEBUG("rss::rss destroyed");
}

void rss::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();    
    menu.addAction(ui_.actionDownload);
    //menu.addAction(ui_.actionDownloadTo);
    //menu.addSeparator();    
    menu.addAction(ui_.actionSave);
    menu.addSeparator();        
    menu.addAction(ui_.actionOpen);
    menu.addSeparator();
    menu.addAction(ui_.actionSettings);
    menu.addSeparator();        
    menu.addAction(ui_.actionStop);

}

void rss::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionRefresh);
    bar.addSeparator();
    bar.addAction(ui_.actionDownload);
    bar.addSeparator();    
    bar.addAction(ui_.actionSave);
    bar.addSeparator(); 
    bar.addAction(ui_.actionOpen);
    bar.addSeparator();   
    bar.addAction(ui_.actionSettings);
    bar.addSeparator();    
    bar.addAction(ui_.actionStop);    
}

void rss::add_settings(std::vector<std::unique_ptr<settings>>& pages)
{
    pages.emplace_back(new rss_feeds_settings_page(feeds_));
    pages.emplace_back(new rss_nzbs_settings_page(nzbs_));
}

void rss::apply_settings()
{
    ui_.btnNzbs->setVisible(nzbs_.enabled);

    if (nzbs_.enabled)
    {
        QVariantMap params;
        params["userid"]   = nzbs_.userid;
        params["apikey"]   = nzbs_.apikey;
        model_.set_params("nzbs", params);
    }
}

void rss::activate(QWidget*)
{}

void rss::save(app::datastore& store) 
{
    store.set("rss", "music", ui_.chkMusic->isChecked());
    store.set("rss", "movies", ui_.chkMovies->isChecked());
    store.set("rss", "tv", ui_.chkTV->isChecked());
    store.set("rss", "console", ui_.chkConsole->isChecked());
    store.set("rss", "computer", ui_.chkComputer->isChecked());
    store.set("rss", "xxx", ui_.chkXXX->isChecked());
    store.set("rss", "feeds", feeds_);
    store.set("rss", "feeds", qlonglong(feeds_));
    store.set("rss", "womble_active", ui_.btnWomble->isChecked());
    store.set("rss", "nzbs_active", ui_.btnNzbs->isChecked());

    store.set("rss", "nzbs_apikey", nzbs_.apikey);
    store.set("rss", "nzbs_userid", nzbs_.userid);
    store.set("rss", "nzbs_enabled", nzbs_.enabled);
    //store.set("rss", "nzbs_feedsize", nzbs_.feedsize);

    const auto* model = ui_.tableView->model();

    // the last column has auto-stretch flag set so it's width
    // is implied. and in fact setting the width will cause rendering bugs
    for (int i=0; i<model->columnCount()-1; ++i)
    {
        const auto width = ui_.tableView->columnWidth(i);
        const auto name  = QString("table_col_%1_width").arg(i);
        store.set("rss", name, width);
    }
}

void rss::load(const app::datastore& store)
{
    ui_.chkMusic->setChecked(store.get("rss", "music", false));
    ui_.chkMovies->setChecked(store.get("rss", "movies", false));
    ui_.chkTV->setChecked(store.get("rss", "tv", false));
    ui_.chkConsole->setChecked(store.get("rss", "console", false));
    ui_.chkComputer->setChecked(store.get("rss", "computer", false));
    ui_.chkXXX->setChecked(store.get("rss", "xxx", false));

    feeds_ = BITFLAG(store.get("rss", "feeds", qlonglong(app::media::all)));

    nzbs_.apikey = store.get("rss", "nzbs_apikey", "");
    nzbs_.userid = store.get("rss", "nzbs_userid", "");
    nzbs_.enabled = store.get("rss", "nzbs_enabled", false);
    //nzbs_.feedsize = store.get("rss", "nzbs_feesize", 25);

    QVariantMap params;
    params["userid"]   = nzbs_.userid;
    params["apikey"]   = nzbs_.apikey;
    //params["feedsize"] = nzbs_.feedsize;
    model_.set_params("nzbs", params);

    ui_.btnNzbs->setVisible(nzbs_.enabled);
    ui_.btnNzbs->setChecked(store.get("rss", "nzbs_active", false));
    ui_.btnWomble->setChecked(store.get("rss", "womble_active", false));

    const auto* model = ui_.tableView->model();

    // see the comments about the columnCount in save()
    for (int i=0; i<model->columnCount()-1; ++i)
    {
        const auto name  = QString("table_col_%1_width").arg(i);
        const auto width = store.get("rss", name, ui_.tableView->columnWidth(i));
        ui_.tableView->setColumnWidth(i, width);
    }
}

mainwidget::info rss::information() const
{
    return {"rss.html", true};
}



void rss::download_selected(const QString& folder)
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;


}

void rss::on_actionRefresh_triggered()
{
    const bool enable_womble = ui_.btnWomble->isChecked();
    const bool enable_nzbs   = ui_.btnNzbs->isChecked() && nzbs_.enabled;
    model_.enable("womble", enable_womble);
    model_.enable("nzbs", enable_nzbs);

    struct feed {
        QCheckBox* chk;
        app::bitflag_t mask;
    } selected_feeds[] = {
        {ui_.chkMusic, BITFLAG(app::media::audio)},
        {ui_.chkMovies, BITFLAG(app::media::movies)},
        {ui_.chkTV, BITFLAG(app::media::tv)},
        {ui_.chkConsole, BITFLAG(app::media::console)},
        {ui_.chkComputer, BITFLAG(app::media::apps)},
        {ui_.chkXXX, BITFLAG(app::media::xxx)}
    };

    bool have_feeds = false;
    bool have_selections = false;

    for (const auto& feed : selected_feeds)
    {
        if (!feed.chk->isChecked())
            continue;

        auto beg = app::media_iterator::begin();
        auto end = app::media_iterator::end();
        for (; beg != end; ++beg)
        {
            const auto type = *beg;
            if (!(type & feed.mask))
                continue;
            if (!(type & feeds_))
                continue;
            if (model_.refresh(type))
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
        msg.setText("There are no feeds available matching the selected categories or you haven't selected any sites");
        msg.exec();
        return;
    }

    model_.clear();

    ui_.progressBar->setVisible(true);
    ui_.actionDownload->setEnabled(false);
    ui_.actionDownloadTo->setEnabled(false);
    ui_.actionStop->setEnabled(true);
}

void rss::on_actionDownload_triggered()
{
    DEBUG("download");
}

void rss::on_actionDownloadTo_triggered()
{
    const auto& folder = win_.select_download_folder();
    if (folder.isEmpty())
        return;

    download_selected(folder);
}

void rss::on_actionSave_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto& folder = win_.select_save_nzb_folder();
    if (folder.isEmpty())
        return;

    bool have_actions = false;

    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        if (model_.save_nzb(row, folder))
            have_actions = true;
    }

    if (!have_actions)
        return;

    ui_.progressBar->setVisible(true);
    ui_.actionStop->setEnabled(true);
}

void rss::on_actionOpen_triggered()
{
    DEBUG("open");
}

void rss::on_actionSettings_triggered()
{
    win_.show_setting("RSS");
}

void rss::on_actionStop_triggered()
{
    model_.stop();
}

void rss::on_actionBrowse_triggered()
{
    const auto& folder = win_.select_download_folder();
    if (folder.isEmpty())
        return;

    download_selected(folder);    
}

void rss::on_tableView_customContextMenuRequested(QPoint point)
{
    QMenu sub("Download to");
    sub.setIcon(QIcon(":/ico/ico_download.png"));

    QStringList paths;
    win_.recents(paths);
    for (const auto& path : paths)
    {
        QAction* action = sub.addAction(QIcon(":/ico/ico_folder.png"),            
            path);
        QObject::connect(action, SIGNAL(triggered(bool)),
            this, SLOT(downloadToPrevious()));
    }

    sub.addSeparator();
    sub.addAction(ui_.actionBrowse);
    sub.setEnabled(ui_.actionDownload->isEnabled());

    QMenu menu;
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();
    menu.addAction(ui_.actionDownload);
    menu.addMenu(&sub);
    menu.addSeparator();
    menu.addAction(ui_.actionSave);
    menu.addSeparator();
    menu.addAction(ui_.actionOpen);
    menu.addSeparator();
    menu.addAction(ui_.actionStop);
    menu.addSeparator();
    menu.addAction(ui_.actionSettings);
    menu.exec(QCursor::pos());
}
 
void rss::ready()
{
    ui_.progressBar->hide();
    ui_.actionStop->setEnabled(false);
}


void rss::rowChanged()
{
    const auto list = ui_.tableView->selectionModel()->selectedRows();
    if (list.isEmpty())
        return;

    ui_.actionDownload->setEnabled(true);
    ui_.actionSave->setEnabled(true);
    ui_.actionOpen->setEnabled(true);
}

void rss::downloadToPrevious()
{
    const auto* action = qobject_cast<const QAction*>(sender());

    // use the directory from the action title
    const auto folder = action->text();

    download_selected(folder);
}

} // gui

