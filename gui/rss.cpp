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

#define LOGTAG "rss"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QMessageBox>
#  include "ui_rss_settings.h"
#include "newsflash/warnpop.h"

#include "rss.h"
#include "mainwindow.h"
#include "nzbfile.h"
#include "common.h"
#include "newznab.h"
#include "dlgmovie.h"
#include "app/debug.h"
#include "app/format.h"
#include "app/eventlog.h"
#include "app/settings.h"
#include "app/utility.h"
#include "app/historydb.h"

namespace gui
{

RSS::RSS(Newznab& module) : module_(module)
{
    ui_.setupUi(this);
    ui_.tableView->setModel(model_.getModel());
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
    //ui_.tableView->setColumnWidth((int)app::RSSReader::columns::locked, 32);

    // when the model has no more actions we hide the progress bar and disable
    // the stop button.
    model_.OnReadyCallback = [&]() {
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
    bar.addAction(ui_.actionSettings);
    bar.addSeparator();
    bar.addAction(ui_.actionStop);
}

void RSS::activate(QWidget*)
{
    if (model_.isEmpty())
    {
        const bool refreshing = ui_.progressBar->isVisible();
        if (refreshing)
            return;
        refreshStreams(false);
    }
}

void RSS::saveState(app::Settings& settings)
{
    app::saveState("rss", ui_.chkAdult, settings);
    app::saveState("rss", ui_.chkApps, settings);
    app::saveState("rss", ui_.chkGames, settings);
    app::saveState("rss", ui_.chkMovies, settings);
    app::saveState("rss", ui_.chkMusic, settings);
    app::saveState("rss", ui_.chkTV, settings);
    app::saveTableLayout("rss", ui_.tableView, settings);
}

void RSS::shutdown()
{
    // we're shutting down. cancel all pending requests if any.
    model_.stop();
}

void RSS::loadState(app::Settings& settings)
{
    app::loadState("rss", ui_.chkAdult, settings);
    app::loadState("rss", ui_.chkApps, settings);
    app::loadState("rss", ui_.chkGames, settings);
    app::loadState("rss", ui_.chkMovies, settings);
    app::loadState("rss", ui_.chkMusic, settings);
    app::loadState("rss", ui_.chkTV, settings);
    app::loadTableLayout("rss", ui_.tableView, settings);
}

MainWidget::info RSS::getInformation() const
{
    return {"rss.html", true};
}

Finder* RSS::getFinder()
{
    return this;
}

void RSS::firstLaunch()
{
    show_popup_hint_ = true;
}

bool RSS::isMatch(const QString& str, std::size_t index, bool caseSensitive)
{
    const auto& item = model_.getItem(index);
    if (!caseSensitive)
    {
        auto upper = item.title.toUpper();
        if (upper.indexOf(str) != -1)
            return true;
    }
    else
    {
        if (item.title.indexOf(str) != -1)
            return true;
    }
    return false;
}

bool RSS::isMatch(const QRegExp& regex, std::size_t index)
{
    const auto& item = model_.getItem(index);
    if (regex.indexIn(item.title) != -1)
        return true;
    return false;
}

std::size_t RSS::numItems() const
{
    return model_.numItems();
}

std::size_t RSS::curItem() const
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return 0;
    const auto& first = indices.first();
    return first.row();
}

void RSS::setFound(std::size_t index)
{
    auto* model = ui_.tableView->model();
    auto i = model->index(index, 0);
    ui_.tableView->setCurrentIndex(i);
    ui_.tableView->scrollTo(i);
}

void RSS::updateBackendList(const QStringList& names)
{
    ui_.cmbServerList->clear();
    for (const auto& name : names)
        ui_.cmbServerList->addItem(name);
}

bool RSS::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        popup_.start(1000);
    }
    else if (event->type() == QEvent::MouseButtonPress)
    {
        if (movie_)
            movie_->hide();
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

        if (!passDuplicateCheck(this, desc, item.type))
            continue;

        if (!passSpaceCheck(this, desc, folder, item.size, item.size, item.type))
            continue;

        const auto acc = selectAccount(this, desc);
        if (acc == 0)
            continue;

        model_.downloadNzbContent(row, acc, folder);
        actions = true;
    }

    if (actions)
    {
        ui_.progressBar->setVisible(true);
        ui_.actionStop->setEnabled(true);
    }
}

void RSS::refreshStreams(bool verbose)
{
    const auto index = ui_.cmbServerList->currentIndex();
    if (index == -1)
    {
        if (verbose)
        {
            QMessageBox msg(this);
            msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msg.setIcon(QMessageBox::Information);
            msg.setText(tr("It looks like you don't have any RSS sites configured.\n"
                "Would you like to configure sites now?"));
            if (msg.exec() == QMessageBox::No)
                return;

            emit showSettings("Newznab");
        }
        return;
    }
    const auto& name = ui_.cmbServerList->currentText();

    auto engine = module_.makeRSSFeedEngine(name);
    model_.setEngine(std::move(engine));

    struct feed {
        QCheckBox* chk;
        app::MainMediaType type;
    } selected_feeds[] = {
        {ui_.chkGames,  app::MainMediaType::Games},
        {ui_.chkMusic,  app::MainMediaType::Music},
        {ui_.chkMovies, app::MainMediaType::Movies},
        {ui_.chkTV,     app::MainMediaType::Television},
        {ui_.chkApps,   app::MainMediaType::Apps},
        {ui_.chkAdult,  app::MainMediaType::Adult}
    };
    bool have_feeds = false;
    bool have_selections = false;

    for (const auto& feed : selected_feeds)
    {
        if (!feed.chk->isChecked())
            continue;

        if (model_.refresh(feed.type))
            have_feeds = true;

        have_selections = true;
    }

    if (!have_selections)
    {
        if (verbose)
        {
            QMessageBox msg(this);
            msg.setStandardButtons(QMessageBox::Ok);
            msg.setIcon(QMessageBox::Information);
            msg.setText("You haven't selected any Media categories.\r\n"
                "Select the sub-categories in Newznab settings and main categories in the RSS main window");
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
            msg.setText("There are no RSS feeds available matching the selected categories.");
            msg.exec();
        }
        return;
    }

    ui_.progressBar->setVisible(true);
    ui_.actionDownload->setEnabled(false);
    ui_.actionDownloadTo->setEnabled(false);
    ui_.actionSave->setEnabled(false);
    ui_.actionOpen->setEnabled(false);
    ui_.actionStop->setEnabled(true);
    ui_.actionRefresh->setEnabled(false);
    ui_.actionInformation->setEnabled(false);
}

void RSS::on_actionRefresh_triggered()
{
    refreshStreams(true);
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
        model_.downloadNzbFile(indices[i].row(), nzb);
        ui_.progressBar->setVisible(true);
        ui_.actionStop->setEnabled(true);
    }
}

void RSS::on_actionOpen_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    static auto callback = [=](const QByteArray& bytes, const QString& desc, app::MediaType type) {
        auto* view = new NZBFile(type);
        view->setProperty("parent-object", QVariant::fromValue(static_cast<QObject*>(this)));
        g_win->attach(view, false);
        view->open(bytes, desc);
    };

    for (int i=0; i<indices.size(); ++i)
    {
        const auto  row  = indices[i].row();
        const auto& item = model_.getItem(row);
        const auto& desc = item.title;
        const auto type  = item.type;
        model_.downloadNzbFile(row, std::bind(callback, std::placeholders::_1, desc, type));
    }

    ui_.progressBar->setVisible(true);
    ui_.actionStop->setEnabled(true);
}

void RSS::on_actionSettings_triggered()
{
    emit showSettings("Newznab");
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

void RSS::on_actionInformation_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto& first = indices[0];
    const auto& item  = model_.getItem(first);

    if (isMovie(item.type))
    {
        const auto& title = app::findMovieTitle(item.title);
        if (title.isEmpty())
            return;
        if (!movie_)
        {
            movie_.reset(new DlgMovie(this));
            QObject::connect(movie_.get(), SIGNAL(startMovieDownload(const QString&)),
                this, SLOT(startMovieDownload(const QString&)));
        }
        movie_->lookupMovie(title, item.guid);
    }
    else if (isTelevision(item.type))
    {
        const auto& title = app::findTVSeriesTitle(item.title);
        if (title.isEmpty())
            return;
        // the same dialog + api can be used for TV series.
        if (!movie_)
        {
            movie_.reset(new DlgMovie(this));
            QObject::connect(movie_.get(), SIGNAL(startMovieDownload(const QString&)),
                this, SLOT(startMovieDownload(const QString&)));
        }
        movie_->lookupSeries(title, item.guid);
    }

    if (show_popup_hint_)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Information),
        msg.setWindowTitle("Hint");
        msg.setText("Did you know that you can also open this dialog by letting the mouse hover over the selected item.");
        msg.exec();
        show_popup_hint_ = false;
    }
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
    menu.addAction(ui_.actionInformation);
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
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    ui_.actionDownload->setEnabled(true);
    ui_.actionSave->setEnabled(true);
    ui_.actionOpen->setEnabled(true);
    ui_.actionInformation->setEnabled(true);

    for (const auto& i : indices)
    {
        const auto& item = model_.getItem(i);
        if (!(isMovie(item.type) || isTelevision(item.type)))
        {
            ui_.actionInformation->setEnabled(false);
            break;
        }
    }
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
    //DEBUG("Popup event!");

    popup_.stop();
    if (movie_ && movie_->isVisible())
        return;

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
    if (isMovie(item.type))
    {
        const auto& title = app::findMovieTitle(item.title);
        if (title.isEmpty())
            return;
        if (!movie_)
        {
            movie_.reset(new DlgMovie(this));
            QObject::connect(movie_.get(), SIGNAL(startMovieDownload(const QString&)),
                this, SLOT(startMovieDownload(const QString&)));
        }
        movie_->lookupMovie(title, item.guid);
    }
    else if (isTelevision(item.type))
    {
        const auto& title = app::findTVSeriesTitle(item.title);
        if (title.isEmpty())
            return;

        if (!movie_)
        {
            movie_.reset(new DlgMovie(this));
            QObject::connect(movie_.get(), SIGNAL(startMovieDownload(const QString&)),
                this, SLOT(startMovieDownload(const QString&)));
        }
        movie_->lookupSeries(title, item.guid);
    }
}

void RSS::startMovieDownload(const QString& guid)
{
    // todo: should actually find the item based on the guid.
    // but for now we assume that the selection cannot change
    // underneath us.
    downloadSelected("");
}

} // gui

