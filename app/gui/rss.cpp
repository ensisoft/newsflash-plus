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

// we can keep the RSS settings in the .cpp because
// it doesn't use signals so it doesn't need MOC to be run (which requires a .h file)
class MySettings : public gui::SettingsWidget
{
public:
    MySettings()
    {
        ui_.setupUi(this);
    }
   ~MySettings()
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
    friend class gui::RSS;
};

} // namespace


namespace gui
{

RSS::RSS()
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

    QObject::connect(&popup_, SIGNAL(timeout()), 
        this, SLOT(popupDetails()));

    ui_.tableView->viewport()->installEventFilter(this);
    ui_.tableView->viewport()->setMouseTracking(true);
    ui_.tableView->setMouseTracking(true);


    // when the model has no more actions we hide the progress bar and disable
    // the stop button.
    model_.on_ready = [&]() {
        ui_.progressBar->hide();
        ui_.actionStop->setEnabled(false);
        ui_.actionRefresh->setEnabled(true);
    };

    DEBUG("RSS gui created");
}

RSS::~RSS()
{
    DEBUG("RSS gui destroyed");
}

void RSS::addActions(QMenu& menu)
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

void RSS::addActions(QToolBar& bar)
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

void RSS::activate(QWidget*)
{
    if (model_.empty())
    {
        const bool refreshing = ui_.progressBar->isVisible();
        if (refreshing)
            return;
        refresh(false);
    }
}

bool RSS::saveState(app::Settings& s)
{
    s.set("rss", "music", ui_.chkMusic->isChecked());
    s.set("rss", "movies", ui_.chkMovies->isChecked());
    s.set("rss", "tv", ui_.chkTV->isChecked());
    s.set("rss", "console", ui_.chkConsole->isChecked());
    s.set("rss", "computer", ui_.chkComputer->isChecked());
    s.set("rss", "xxx", ui_.chkXXX->isChecked());

    s.set("rss", "enable_nzbs", enable_nzbs_);
    s.set("rss", "enable_womble", enable_womble_);
    s.set("rss", "nzbs_apikey", nzbs_apikey_);
    s.set("rss", "nzbs_userid", nzbs_userid_);
    s.set("rss", "streams", quint32(streams_.value()));

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

void RSS::shutdown()
{
    // we're shutting down. cancel all pending requests if any.
    model_.stop();
}

void RSS::loadState(app::Settings& s)
{
    ui_.chkMusic->setChecked(s.get("rss", "music", true));
    ui_.chkMovies->setChecked(s.get("rss", "movies", true));
    ui_.chkTV->setChecked(s.get("rss", "tv", true));
    ui_.chkConsole->setChecked(s.get("rss", "console", true));
    ui_.chkComputer->setChecked(s.get("rss", "computer", true));
    ui_.chkXXX->setChecked(s.get("rss", "xxx", true));

    enable_nzbs_    = s.get("rss", "enable_nzbs", false);
    enable_womble_  = s.get("rss", "enable_womble", true);
    nzbs_apikey_    = s.get("rss", "nzbs_apikey", "");
    nzbs_userid_    = s.get("rss", "nzbs_userid", "");    
    const auto bits = s.get("rss", "streams", quint32(~0));

    streams_.set_from_value(bits); // enable all bits (streams)

    model_.enable_feed("womble", enable_womble_);
    model_.enable_feed("nzbs", enable_nzbs_);
    model_.set_credentials("nzbs", nzbs_userid_, nzbs_apikey_);

    const auto* model = ui_.tableView->model();

    // see the comments about the columnCount in save()
    for (int i=0; i<model->columnCount()-1; ++i)
    {
        const auto name  = QString("table_col_%1_width").arg(i);
        const auto width = s.get("rss", name, ui_.tableView->columnWidth(i));
        ui_.tableView->setColumnWidth(i, width);
    }
}

MainWidget::info RSS::getInformation() const
{
    return {"rss.html", true, true};
}

SettingsWidget* RSS::getSettings()
{
    auto* p = new MySettings;
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
    ui.edtNZBSUserId->setText(nzbs_userid_);
    ui.edtNZBSApikey->setText(nzbs_apikey_);

    return p;
}


void RSS::applySettings(SettingsWidget* gui)
{
    auto* mine = dynamic_cast<MySettings*>(gui);
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
    nzbs_apikey_      = ui.edtNZBSApikey->text();
    nzbs_userid_      = ui.edtNZBSUserId->text();

    model_.enable_feed("womble", enable_womble_);
    model_.enable_feed("nzbs", enable_nzbs_);
    model_.set_credentials("nzbs", nzbs_userid_, nzbs_apikey_);    
}

void RSS::freeSettings(SettingsWidget* s)
{
    delete s;
}

bool RSS::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        popup_.start(1000);
        //if (movie_)
        //    movie_->hide();
    }
    return QObject::eventFilter(obj, event);
}

void RSS::downloadSelected(const QString& folder)
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    bool actions = false;

    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        const auto& item = model_.getItem(row);
        const auto& desc = item.title;
        const auto acc = g_win->chooseAccount(desc);
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

void RSS::refresh(bool verbose)
{
    if (!enable_nzbs_ && !enable_womble_)
    {
        if (verbose)
        {
            QMessageBox msg(this);
            msg.setStandardButtons(QMessageBox::Ok);
            msg.setIcon(QMessageBox::Information);
            msg.setText(tr("You haven't enabled any RSS feed sites.\n\rYou can enable them in the RSS settings."));
            msg.exec();
        }
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
        if (verbose)
        {
            QMessageBox msg(this);
            msg.setStandardButtons(QMessageBox::Ok);
            msg.setIcon(QMessageBox::Information);
            msg.setText("You haven't selected any RSS Media categories.\r\n" 
                "Select the sub-categories in RSS settings and main categories in the RSS main window");
            msg.exec();
        }
        return; 
    }
    else if (!have_feeds)
    {
        if (verbose)
        {
            QMessageBox msg(this);
            msg.setStandardButtons(QMessageBox::Ok);
            msg.setIcon(QMessageBox::Information);
            msg.setText("There are no feeds available matching the selected categories.");
            msg.exec();
        }
        return;
    }

    model_.clear();

    ui_.progressBar->setVisible(true);
    ui_.actionDownload->setEnabled(false);
    ui_.actionDownloadTo->setEnabled(false);
    ui_.actionStop->setEnabled(true);    
    ui_.actionRefresh->setEnabled(false);
}

void RSS::on_actionRefresh_triggered()
{
    refresh(true);
}

void RSS::on_actionDownload_triggered()
{
    downloadSelected("");
}

void RSS::on_actionDownloadTo_triggered()
{
    const auto& folder = g_win->selectDownloadFolder();
    if (folder.isEmpty())
        return;

    downloadSelected(folder);
}

void RSS::on_actionSave_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    for (int i=0; i<indices.size(); ++i)
    {
        const auto& item = model_.getItem(indices[i].row());
        const auto& nzb  = g_win->selectNzbSaveFile(item.title + ".nzb");
        if (nzb.isEmpty())
            continue;
        model_.download_nzb_file(indices[i].row(), nzb);
    }
    ui_.progressBar->setVisible(true);
    ui_.actionStop->setEnabled(true);
}

void RSS::on_actionOpen_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    static auto callback = [](const QByteArray& bytes, const QString& desc) {
        auto* view = new NZBFile();
        g_win->attach(view, true);
        view->open(bytes, desc);
    };

    for (int i=0; i<indices.size(); ++i)
    {
        const auto  row  = indices[i].row();
        const auto& item = model_.getItem(row);
        const auto& desc = item.title;
        model_.view_nzb_content(row, std::bind(callback,
            std::placeholders::_1, desc));
    }

    ui_.progressBar->setVisible(true);
    ui_.actionStop->setEnabled(true);
}

void RSS::on_actionSettings_triggered()
{
    g_win->showSetting("RSS");
}

void RSS::on_actionStop_triggered()
{
    model_.stop();
    ui_.progressBar->hide();
    ui_.actionStop->setEnabled(false);    
    ui_.actionRefresh->setEnabled(true);
}

void RSS::on_actionBrowse_triggered()
{
    const auto& folder = g_win->selectDownloadFolder();
    if (folder.isEmpty())
        return;

    downloadSelected(folder);    
}

void RSS::on_tableView_customContextMenuRequested(QPoint point)
{
    QMenu sub("Download to");
    sub.setIcon(QIcon("icons:ico_download.png"));

    const auto& recents = g_win->getRecentPaths();
    for (const auto& recent : recents)
    {
        QAction* action = sub.addAction(QIcon("icons:ico_folder.png"), recent);
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

void RSS::on_tableView_doubleClicked(const QModelIndex&)
{
    on_actionDownload_triggered();
}

void RSS::rowChanged()
{
    const auto list = ui_.tableView->selectionModel()->selectedRows();
    if (list.isEmpty())
        return;

    ui_.actionDownload->setEnabled(true);
    ui_.actionSave->setEnabled(true);
    ui_.actionOpen->setEnabled(true);
}

void RSS::downloadToPrevious()
{
    const auto* action = qobject_cast<const QAction*>(sender());

    // use the directory from the action title
    const auto folder = action->text();

    downloadSelected(folder);
}
void RSS::popupDetails()
{
    DEBUG("Popup event!");

    popup_.stop();

    if (!ui_.tableView->underMouse())
        return;

    const QPoint global = QCursor::pos();
    const QPoint local  = ui_.tableView->viewport()->mapFromGlobal(global);

    const auto& all = ui_.tableView->selectionModel()->selectedRows();
    const auto& sel = ui_.tableView->indexAt(local);
    int i=0;
    for (; i<all.size(); ++i)
    {
        if (all[i].row() == sel.row())
            break;
    }
    if (i == all.size())
        return;

    const auto& item  = model_.getItem(sel.row());
    const auto& title = app::findMovieTitle(item.title);
    if (title.isEmpty())
        return;

    if (!movie_)
    {
        movie_.reset(new DlgMovie(this));
    }    
    movie_->move(QCursor::pos());
    movie_->lookup(title);
}

} // gui

