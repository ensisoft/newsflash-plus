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

#define LOGTAG "rss"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QMessageBox>
#  include "ui_rss_settings.h"
#include <newsflash/warnpop.h>

#include "rss.h"
#include "mainwindow.h"
#include "nzbfile.h"
#include "../debug.h"
#include "../format.h"
#include "../eventlog.h"
#include "../settings.h"

namespace {

class rss_settings : public gui::settings
{
public:
    rss_settings()
    {
        ui_.setupUi(this);
    }
   ~rss_settings()
    {}

    virtual bool validate() const override
    {
        const bool enable_nzbs = ui_.grpNZBS->isChecked();
        if (enable_nzbs)
        {
            const auto userid = ui_.edtNZBSUserId->text();
            if (userid.isEmpty())
            {
                ui_.edtNZBSUserId->setFocus();
                return false;
            }
            const auto apikey = ui_.edtNZBSApikey->text();
            if(apikey.isEmpty())
            {
                ui_.edtNZBSApikey->setFocus();
                return false;                
            }
        }
        return true;
    }

private:
    Ui::RSSSettings ui_;
private:
    friend class gui::rss;
};

} // namespace


namespace gui
{

rss::rss()
{
    ui_.setupUi(this);
    ui_.tableView->setModel(&model_);
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

    // when the model has no more actions we hide the progress bar and disable
    // the stop button.
    model_.on_ready = [&]() {
        ui_.progressBar->hide();
        ui_.actionStop->setEnabled(false);
    };

    DEBUG("rss gui created");
}

rss::~rss()
{
    DEBUG("rss gui destroyed");
}

void rss::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();    
    menu.addAction(ui_.actionDownload);
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

void rss::activate(QWidget*)
{}

bool rss::savestate(app::settings& s)
{
    s.set("rss", "music", ui_.chkMusic->isChecked());
    s.set("rss", "movies", ui_.chkMovies->isChecked());
    s.set("rss", "tv", ui_.chkTV->isChecked());
    s.set("rss", "console", ui_.chkConsole->isChecked());
    s.set("rss", "computer", ui_.chkComputer->isChecked());
    s.set("rss", "xxx", ui_.chkXXX->isChecked());

    const auto* model = ui_.tableView->model();

    // the last column has auto-stretch flag set so it's width
    // is implied. and in fact setting the width will cause rendering bugs
    for (int i=0; i<model->columnCount()-1; ++i)
    {
        const auto width = ui_.tableView->columnWidth(i);
        const auto name  = QString("table_col_%1_width").arg(i);
        s.set("rss", name, width);
    }

    return true;
}

void rss::loadstate(app::settings& s)
{
    ui_.chkMusic->setChecked(s.get("rss", "music", true));
    ui_.chkMovies->setChecked(s.get("rss", "movies", true));
    ui_.chkTV->setChecked(s.get("rss", "tv", true));
    ui_.chkConsole->setChecked(s.get("rss", "console", true));
    ui_.chkComputer->setChecked(s.get("rss", "computer", true));
    ui_.chkXXX->setChecked(s.get("rss", "xxx", true));

    enable_nzbs_    = s.get("rss", "enable_nzbs", false);
    enable_womble_  = s.get("rss", "enable_womble", true);
    const auto bits = s.get("rss", "streams", quint32(~0));

    streams_.set_from_value(bits); // enable all bits (streams)

    const auto* model = ui_.tableView->model();

    // see the comments about the columnCount in save()
    for (int i=0; i<model->columnCount()-1; ++i)
    {
        const auto name  = QString("table_col_%1_width").arg(i);
        const auto width = s.get("rss", name, ui_.tableView->columnWidth(i));
        ui_.tableView->setColumnWidth(i, width);
    }

    model_.enable_feed("womble", enable_womble_);
    model_.enable_feed("nzbs", enable_nzbs_);

    const auto apikey = s.get("rss", "nzbs_apikey", "");
    const auto userid = s.get("rss", "nzbs_userid", "");
    model_.set_credentials("nzbs", userid, apikey);

}

mainwidget::info rss::information() const
{
    return {"rss.html", true, true};
}

gui::settings* rss::get_settings(app::settings& s)
{
    auto* p = new rss_settings;
    auto& ui = p->ui_;

    using m = app::media;
    if (streams_.test(m::tv_int)) ui.chkTvInt->setChecked(true);
    if (streams_.test(m::tv_sd))  ui.chkTvSD->setChecked(true);
    if (streams_.test(m::tv_hd))  ui.chkTvHD->setChecked(true);

    if (streams_.test(m::apps_pc)) ui.chkComputerPC->setChecked(true);
    if (streams_.test(m::apps_iso)) ui.chkComputerISO->setChecked(true);
    if (streams_.test(m::apps_android)) ui.chkComputerAndroid->setChecked(true);
    if (streams_.test(m::apps_mac) || streams_.test(m::apps_ios)) 
        ui.chkComputerMac->setChecked(true);        

    if (streams_.test(m::audio_mp3)) ui.chkMusicMP3->setChecked(true);
    if (streams_.test(m::audio_video)) ui.chkMusicVideo->setChecked(true);
    if (streams_.test(m::audio_lossless)) ui.chkMusicLosless->setChecked(true);

    if (streams_.test(m::movies_int)) ui.chkMoviesInt->setChecked(true);
    if (streams_.test(m::movies_sd)) ui.chkMoviesSD->setChecked(true);
    if (streams_.test(m::movies_hd)) ui.chkMoviesHD->setChecked(true);

    if (streams_.test(m::console_nds) ||
        streams_.test(m::console_wii))
        ui.chkConsoleNintendo->setChecked(true);

    if (streams_.test(m::console_psp) ||
        streams_.test(m::console_ps2) ||
        streams_.test(m::console_ps3) ||
        streams_.test(m::console_ps4))
        ui.chkConsolePlaystation->setChecked(true);

    if (streams_.test(m::console_xbox) ||
        streams_.test(m::console_xbox360))
        ui.chkConsoleXbox->setChecked(true);

    if (streams_.test(m::xxx_dvd)) ui.chkXXXDVD->setChecked(true);
    if (streams_.test(m::xxx_sd)) ui.chkXXXSD->setChecked(true);
    if (streams_.test(m::xxx_hd)) ui.chkXXXHD->setChecked(true);

    ui.grpWomble->setChecked(enable_womble_);
    ui.grpNZBS->setChecked(enable_nzbs_);
    ui.edtNZBSUserId->setText(s.get("rss", "nzbs_userid", ""));
    ui.edtNZBSApikey->setText(s.get("rss", "nzbs_apikey", ""));

    return p;
}


void rss::apply_settings(gui::settings* gui, app::settings& backend)
{
    auto* mine = dynamic_cast<rss_settings*>(gui);
    auto& ui = mine->ui_;

    using m = app::media;

    streams_.clear();

    if (ui.chkTvInt->isChecked()) streams_.set(m::tv_int);
    if (ui.chkTvSD->isChecked()) streams_.set(m::tv_sd);
    if (ui.chkTvHD->isChecked()) streams_.set(m::tv_hd);

    if (ui.chkComputerPC->isChecked()) streams_.set(m::apps_pc);
    if (ui.chkComputerISO->isChecked()) streams_.set(m::apps_iso);
    if (ui.chkComputerAndroid->isChecked()) streams_.set(m::apps_android);
    if (ui.chkComputerMac->isChecked()) {
        streams_.set(m::apps_mac);
        streams_.set(m::apps_ios);
    }

    if (ui.chkMusicMP3->isChecked()) streams_.set(m::audio_mp3);
    if (ui.chkMusicVideo->isChecked()) streams_.set(m::audio_video);
    if (ui.chkMusicLosless->isChecked()) streams_.set(m::audio_lossless);

    if (ui.chkMoviesInt->isChecked()) streams_.set(m::movies_int);
    if (ui.chkMoviesSD->isChecked()) streams_.set(m::movies_sd);
    if (ui.chkMoviesHD->isChecked()) streams_.set(m::movies_hd);

    if (ui.chkConsoleNintendo->isChecked())
    {
        streams_.set(m::console_nds);
        streams_.set(m::console_wii);
    }
    if (ui.chkConsolePlaystation->isChecked())
    {
        streams_.set(m::console_psp);
        streams_.set(m::console_ps2);
        streams_.set(m::console_ps3);
        streams_.set(m::console_ps4);
    }
    if (ui.chkConsoleXbox->isChecked())
    {
        streams_.set(m::console_xbox);
        streams_.set(m::console_xbox360);
    }

    if (ui.chkXXXDVD->isChecked()) streams_.set(m::xxx_dvd);
    if (ui.chkXXXHD->isChecked()) streams_.set(m::xxx_hd);
    if (ui.chkXXXSD->isChecked()) streams_.set(m::xxx_sd);

    enable_womble_    = ui.grpWomble->isChecked();
    enable_nzbs_      = ui.grpNZBS->isChecked();
    const auto apikey = ui.edtNZBSApikey->text();
    const auto userid = ui.edtNZBSUserId->text();

    backend.set("rss", "nzbs_apikey", apikey);
    backend.set("rss", "nzbs_userid", userid);
    backend.set("rss", "enable_nzbs",   enable_nzbs_);
    backend.set("rss", "enable_womble", enable_womble_);
    backend.set("rss", "streams", quint32(streams_.value()));

    model_.enable_feed("womble", enable_womble_);
    model_.enable_feed("nzbs", enable_nzbs_);
    model_.set_credentials("nzbs", userid, apikey);    
}

void rss::free_settings(gui::settings* s)
{
    delete s;
}


void rss::download_selected(const QString& folder)
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    bool actions = false;

    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        const auto& item = model_.get_item(row);
        const auto& desc = item.title;
        const auto acc = g_win->choose_account(desc);
        if (acc == 0)
            continue;

        model_.download_nzb_content(row, acc, folder);
        actions = true;
    }

    if (actions)
    {
        ui_.progressBar->setVisible(true);        
        ui_.actionStop->setEnabled(true);        
    }
}

void rss::on_actionRefresh_triggered()
{
    if (!enable_nzbs_ && !enable_womble_)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Information);
        msg.setText(tr("You haven't enabled any RSS feed sites.\n\rYou can enable them in the RSS settings."));
        msg.exec();
        return;       
    }

    using m = app::media;
    using s = newsflash::bitflag<app::media>;

//    const auto foo = s(m::audio_mp3) | m::audio_lossless;

    const auto audio   = s(m::audio_mp3) | s(m::audio_lossless) | s(m::audio_video);
    const auto video   = s(m::movies_int) | s(m::movies_sd) | s(m::movies_hd);
    const auto tv = s(m::tv_int) | s(m::tv_hd) | s(m::tv_sd);    
    const auto computer = s(m::apps_pc) | s(m::apps_iso) | s(m::apps_android) | s(m::apps_ios) | s(m::apps_mac);
    const auto xxx = s(m::xxx_dvd) | s(m::xxx_hd) | s(m::xxx_sd);
    const auto console = s(m::console_psp) | s(m::console_ps2) | s(m::console_ps3) | s(m::console_ps4) |
        s(m::console_xbox) | s(m::console_xbox360) | s(m::console_nds) | s(m::console_wii);

    struct feed {
        QCheckBox* chk;
        s mask;
    } selected_feeds[] = {
        {ui_.chkConsole, console},        
        {ui_.chkMusic, audio},
        {ui_.chkMovies, video},
        {ui_.chkTV, tv},
        {ui_.chkComputer, computer},
        {ui_.chkXXX, xxx}
    };

    bool have_selections = false;
    bool have_feeds = false;

    for (const auto& feed : selected_feeds)
    {
        if (!feed.chk->isChecked())
            continue;

        // iterate all the bits (streams) and start RSS refresh operations
        // on all bits that are set.
        auto beg = app::media_iterator::begin();
        auto end = app::media_iterator::end();
        for (; beg != end; ++beg)
        {
            const auto m = *beg;
            if (!feed.mask.test(m))
                continue;
            if (!streams_.test(m))
                continue;

            if (model_.refresh(m))
                have_feeds = true;

            have_selections = true;
        }
    }

    if (!have_selections)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Information);
        msg.setText("You haven't selected any RSS Media categories.\r\n" 
                    "Select the sub-categories in RSS settings and main categories in the RSS main window");
        msg.exec();
        return; 
    }
    else if (!have_feeds)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Information);
        msg.setText("There are no feeds available matching the selected categories.");
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
    download_selected("");
}

void rss::on_actionDownloadTo_triggered()
{
    const auto& folder = g_win->select_download_folder();
    if (folder.isEmpty())
        return;

    download_selected(folder);
}

void rss::on_actionSave_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    for (int i=0; i<indices.size(); ++i)
    {
        const auto& item = model_.get_item(indices[i].row());
        const auto& nzb  = g_win->select_nzb_save_file(item.title + ".nzb");
        if (nzb.isEmpty())
            continue;
        model_.download_nzb_file(indices[i].row(), nzb);
    }
    ui_.progressBar->setVisible(true);
    ui_.actionStop->setEnabled(true);
}

void rss::on_actionOpen_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    static auto callback = [](const QByteArray& bytes, const QString& desc) {
        auto* view = new nzbfile();
        g_win->attach(view);
        view->open(bytes, desc);
    };

    for (int i=0; i<indices.size(); ++i)
    {
        const auto  row  = indices[i].row();
        const auto& item = model_.get_item(row);
        const auto& desc = item.title;
        model_.view_nzb_content(row, std::bind(callback,
            std::placeholders::_1, desc));
    }

    ui_.progressBar->setVisible(true);
    ui_.actionStop->setEnabled(true);
}

void rss::on_actionSettings_triggered()
{
    g_win->show_setting("RSS");
}

void rss::on_actionStop_triggered()
{
    model_.stop();
}

void rss::on_actionBrowse_triggered()
{
    const auto& folder = g_win->select_download_folder();
    if (folder.isEmpty())
        return;

    download_selected(folder);    
}

void rss::on_tableView_customContextMenuRequested(QPoint point)
{
    QMenu sub("Download to");
    sub.setIcon(QIcon(":/ico/ico_download.png"));

    QStringList paths = g_win->get_recent_paths();
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

