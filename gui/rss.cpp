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

RSS::RSS(Newznab& module) : mNewznab(module)
{
    mUi.setupUi(this);
    mUi.tableView->setModel(mRSSReader.getModel());
    mUi.actionDownload->setEnabled(false);
    mUi.actionDownloadTo->setEnabled(false);
    mUi.actionSave->setEnabled(false);
    mUi.actionOpen->setEnabled(false);
    mUi.actionStop->setEnabled(false);
    mUi.progressBar->setVisible(false);
    mUi.progressBar->setValue(0);
    mUi.progressBar->setRange(0, 0);

    auto* selection = mUi.tableView->selectionModel();
    QObject::connect(selection, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(rowChanged()));

    QObject::connect(&mPopupTimer, SIGNAL(timeout()),
        this, SLOT(popupDetails()));

    mUi.tableView->viewport()->installEventFilter(this);
    mUi.tableView->viewport()->setMouseTracking(true);
    mUi.tableView->setMouseTracking(true);
    //mUi.tableView->setColumnWidth((int)app::RSSReader::columns::locked, 32);

    // when the model has no more actions we hide the progress bar and disable
    // the stop button.
    mRSSReader.OnReadyCallback = [&]() {
        mUi.progressBar->hide();
        mUi.actionStop->setEnabled(false);
        mUi.actionRefresh->setEnabled(true);
    };

    DEBUG("RSS gui created");
}

RSS::~RSS()
{
    DEBUG("RSS gui destroyed");
}

void RSS::addActions(QMenu& menu)
{
    menu.addAction(mUi.actionRefresh);
    menu.addSeparator();
    menu.addAction(mUi.actionDownload);
    menu.addAction(mUi.actionSave);
    menu.addSeparator();
    menu.addAction(mUi.actionOpen);
    menu.addSeparator();
    menu.addAction(mUi.actionSettings);
    menu.addSeparator();
    menu.addAction(mUi.actionStop);
}

void RSS::addActions(QToolBar& bar)
{
    bar.addAction(mUi.actionRefresh);
    bar.addSeparator();
    bar.addAction(mUi.actionDownload);
    bar.addSeparator();
    bar.addAction(mUi.actionSettings);
    bar.addSeparator();
    bar.addAction(mUi.actionStop);
}

void RSS::activate(QWidget*)
{
    if (mRSSReader.isEmpty())
    {
        const bool refreshing = mUi.progressBar->isVisible();
        if (refreshing)
            return;
        refreshStreams(false);
    }
}

void RSS::saveState(app::Settings& settings)
{
    app::saveState("rss", mUi.chkAdult, settings);
    app::saveState("rss", mUi.chkApps, settings);
    app::saveState("rss", mUi.chkGames, settings);
    app::saveState("rss", mUi.chkMovies, settings);
    app::saveState("rss", mUi.chkMusic, settings);
    app::saveState("rss", mUi.chkTV, settings);
    app::saveTableLayout("rss", mUi.tableView, settings);
}

void RSS::shutdown()
{
    // we're shutting down. cancel all pending requests if any.
    mRSSReader.stop();
}

void RSS::loadState(app::Settings& settings)
{
    app::loadState("rss", mUi.chkAdult, settings);
    app::loadState("rss", mUi.chkApps, settings);
    app::loadState("rss", mUi.chkGames, settings);
    app::loadState("rss", mUi.chkMovies, settings);
    app::loadState("rss", mUi.chkMusic, settings);
    app::loadState("rss", mUi.chkTV, settings);
    app::loadTableLayout("rss", mUi.tableView, settings);
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
    mShowPopupHint = true;
}

bool RSS::isMatch(const QString& str, std::size_t index, bool caseSensitive)
{
    const auto& item = mRSSReader.getItem(index);
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
    const auto& item = mRSSReader.getItem(index);
    if (regex.indexIn(item.title) != -1)
        return true;
    return false;
}

std::size_t RSS::numItems() const
{
    return mRSSReader.numItems();
}

std::size_t RSS::curItem() const
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return 0;
    const auto& first = indices.first();
    return first.row();
}

void RSS::setFound(std::size_t index)
{
    auto* model = mUi.tableView->model();
    auto i = model->index(index, 0);
    mUi.tableView->setCurrentIndex(i);
    mUi.tableView->scrollTo(i);
}

void RSS::updateBackendList(const QStringList& names)
{
    mUi.cmbServerList->clear();
    for (const auto& name : names)
        mUi.cmbServerList->addItem(name);
}

bool RSS::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        mPopupTimer.start(1000);
    }
    else if (event->type() == QEvent::MouseButtonPress)
    {
        if (mMovieDetails)
            mMovieDetails->hide();
    }
    return QObject::eventFilter(obj, event);
}

void RSS::downloadSelected(const QString& folder)
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    bool actions = false;

    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        const auto& item = mRSSReader.getItem(row);
        const auto& desc = item.title;

        if (!passDuplicateCheck(this, desc, item.type))
            continue;

        if (!passSpaceCheck(this, desc, folder, item.size, item.size, item.type))
            continue;

        const auto acc = selectAccount(this, desc);
        if (acc == 0)
            continue;

        mRSSReader.downloadNzbContent(row, acc, folder);
        actions = true;
    }

    if (actions)
    {
        mUi.progressBar->setVisible(true);
        mUi.actionStop->setEnabled(true);
    }
}

void RSS::refreshStreams(bool verbose)
{
    const auto index = mUi.cmbServerList->currentIndex();
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
    const auto& name = mUi.cmbServerList->currentText();

    auto engine = mNewznab.makeRSSFeedEngine(name);
    mRSSReader.setEngine(std::move(engine));

    struct feed {
        QCheckBox* chk;
        app::MainMediaType type;
    } selected_feeds[] = {
        {mUi.chkGames,  app::MainMediaType::Games},
        {mUi.chkMusic,  app::MainMediaType::Music},
        {mUi.chkMovies, app::MainMediaType::Movies},
        {mUi.chkTV,     app::MainMediaType::Television},
        {mUi.chkApps,   app::MainMediaType::Apps},
        {mUi.chkAdult,  app::MainMediaType::Adult}
    };
    bool have_feeds = false;
    bool have_selections = false;

    for (const auto& feed : selected_feeds)
    {
        if (!feed.chk->isChecked())
            continue;

        if (mRSSReader.refresh(feed.type))
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

    mUi.progressBar->setVisible(true);
    mUi.actionDownload->setEnabled(false);
    mUi.actionDownloadTo->setEnabled(false);
    mUi.actionSave->setEnabled(false);
    mUi.actionOpen->setEnabled(false);
    mUi.actionStop->setEnabled(true);
    mUi.actionRefresh->setEnabled(false);
    mUi.actionInformation->setEnabled(false);
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
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    for (int i=0; i<indices.size(); ++i)
    {
        const auto& item = mRSSReader.getItem(indices[i].row());
        const auto& nzb  = g_win->selectNzbSaveFile(item.title + ".nzb");
        if (nzb.isEmpty())
            continue;
        mRSSReader.downloadNzbFile(indices[i].row(), nzb);
        mUi.progressBar->setVisible(true);
        mUi.actionStop->setEnabled(true);
    }
}

void RSS::on_actionOpen_triggered()
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    static auto callback = [=](const QByteArray& bytes, const QString& desc, app::MediaType type) {
        auto* view = new NZBFile(type);
        view->setProperty("parent-object", QVariant::fromValue(static_cast<QObject*>(this)));
        newsflash::bitflag<MainWindow::WidgetAttachFlags> flags;
        flags.set(MainWindow::WidgetAttachFlags::LoadState);
        flags.set(MainWindow::WidgetAttachFlags::Activate);
        g_win->attach(view, flags);
        view->open(bytes, desc);
    };

    for (int i=0; i<indices.size(); ++i)
    {
        const auto  row  = indices[i].row();
        const auto& item = mRSSReader.getItem(row);
        const auto& desc = item.title;
        const auto type  = item.type;
        mRSSReader.downloadNzbFile(row, std::bind(callback, std::placeholders::_1, desc, type));
    }

    mUi.progressBar->setVisible(true);
    mUi.actionStop->setEnabled(true);
}

void RSS::on_actionSettings_triggered()
{
    emit showSettings("Newznab");
}

void RSS::on_actionStop_triggered()
{
    mRSSReader.stop();
    mUi.progressBar->hide();
    mUi.actionStop->setEnabled(false);
    mUi.actionRefresh->setEnabled(true);
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
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto& first = indices[0];
    const auto& item  = mRSSReader.getItem(first);

    if (isMovie(item.type))
    {
        QString releaseYear;
        const auto& title = app::findMovieTitle(item.title, &releaseYear);
        if (title.isEmpty())
            return;
        if (!mMovieDetails)
        {
            mMovieDetails.reset(new DlgMovie(this));
            QObject::connect(mMovieDetails.get(), SIGNAL(startMovieDownload(const QString&)),
                this, SLOT(startMovieDownload(const QString&)));
        }
        mMovieDetails->lookupMovie(title, item.guid, releaseYear);
    }
    else if (isTelevision(item.type))
    {
        const auto& title = app::findTVSeriesTitle(item.title);
        if (title.isEmpty())
            return;
        // the same dialog + api can be used for TV series.
        if (!mMovieDetails)
        {
            mMovieDetails.reset(new DlgMovie(this));
            QObject::connect(mMovieDetails.get(), SIGNAL(startMovieDownload(const QString&)),
                this, SLOT(startMovieDownload(const QString&)));
        }
        mMovieDetails->lookupSeries(title, item.guid);
    }

    if (mShowPopupHint)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Information),
        msg.setWindowTitle("Hint");
        msg.setText("Did you know that you can also open this dialog by letting the mouse hover over the selected item.");
        msg.exec();
        mShowPopupHint = false;
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
    sub.addAction(mUi.actionBrowse);
    sub.setEnabled(mUi.actionDownload->isEnabled());

    mUi.actionInformation->setEnabled(DlgMovie::isLookupEnabled());

    QMenu menu;
    menu.addAction(mUi.actionRefresh);
    menu.addSeparator();
    menu.addAction(mUi.actionDownload);
    menu.addMenu(&sub);
    menu.addSeparator();
    menu.addAction(mUi.actionSave);
    menu.addSeparator();
    menu.addAction(mUi.actionOpen);
    menu.addSeparator();
    menu.addAction(mUi.actionInformation);
    menu.addSeparator();
    menu.addAction(mUi.actionStop);
    menu.addSeparator();
    menu.addAction(mUi.actionSettings);
    menu.exec(QCursor::pos());
}

void RSS::on_tableView_doubleClicked(const QModelIndex&)
{
    on_actionDownload_triggered();
}

void RSS::rowChanged()
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    mUi.actionDownload->setEnabled(true);
    mUi.actionSave->setEnabled(true);
    mUi.actionOpen->setEnabled(true);
    mUi.actionInformation->setEnabled(true);

    for (const auto& i : indices)
    {
        const auto& item = mRSSReader.getItem(i);
        if (!(isMovie(item.type) || isTelevision(item.type)))
        {
            mUi.actionInformation->setEnabled(false);
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

    mPopupTimer.stop();
    if (!DlgMovie::isLookupEnabled())
        return;

    if (mMovieDetails && mMovieDetails->isVisible())
        return;

    if (!mUi.tableView->underMouse())
        return;

    const QPoint global = QCursor::pos();
    const QPoint local  = mUi.tableView->viewport()->mapFromGlobal(global);

    const auto& all = mUi.tableView->selectionModel()->selectedRows();
    const auto& sel = mUi.tableView->indexAt(local);
    int i=0;
    for (; i<all.size(); ++i)
    {
        if (all[i].row() == sel.row())
            break;
    }
    if (i == all.size())
        return;

    const auto& item  = mRSSReader.getItem(sel.row());
    if (isMovie(item.type))
    {
        QString releaseYear;
        const auto& title = app::findMovieTitle(item.title, &releaseYear);
        if (title.isEmpty())
            return;
        if (!mMovieDetails)
        {
            mMovieDetails.reset(new DlgMovie(this));
            QObject::connect(mMovieDetails.get(), SIGNAL(startMovieDownload(const QString&)),
                this, SLOT(startMovieDownload(const QString&)));
        }
        mMovieDetails->lookupMovie(title, item.guid, releaseYear);
    }
    else if (isTelevision(item.type))
    {
        const auto& title = app::findTVSeriesTitle(item.title);
        if (title.isEmpty())
            return;

        if (!mMovieDetails)
        {
            mMovieDetails.reset(new DlgMovie(this));
            QObject::connect(mMovieDetails.get(), SIGNAL(startMovieDownload(const QString&)),
                this, SLOT(startMovieDownload(const QString&)));
        }
        mMovieDetails->lookupSeries(title, item.guid);
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

