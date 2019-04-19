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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#  include <QtGui/QMessageBox>
#  include <QDate>
#include "newsflash/warnpop.h"

#include "mainwindow.h"
#include "search.h"
#include "newznab.h"
#include "nzbfile.h"
#include "dlgmovie.h"
#include "dlgwildcards.h"
#include "common.h"
#include "app/debug.h"
#include "app/search.h"
#include "app/settings.h"
#include "app/utility.h"
#include "app/engine.h"

namespace gui
{

const auto QuerySize = 100;

Search::Search(Newznab& module) : mNewznab(module)
{
    mUi.setupUi(this);
    mUi.btnBasic->setChecked(true);
    mUi.wBasic->setVisible(true);
    mUi.wAdvanced->setVisible(false);
    mUi.wTV->setVisible(false);
    mUi.wMusic->setVisible(false);
    mUi.tableView->setModel(mSearch.getModel());
    mUi.progress->setVisible(false);
    mUi.progress->setMinimum(0);
    mUi.progress->setMaximum(0);
    mUi.actionStop->setEnabled(false);
    mUi.actionDownload->setEnabled(false);
    mUi.actionSave->setEnabled(false);
    mUi.actionOpen->setEnabled(false);
    mUi.editSearch->setFocus();
    mUi.btnSearchMore->setEnabled(false);
    mUi.editSearch->setFocus();

    mUi.cmbYear->addItem("");
    const auto thisYear = QDate::currentDate().year();
    for (int year=thisYear; year>=1900; --year)
        mUi.cmbYear->addItem(QString::number(year));

    mSearch.OnReadyCallback = [=]() {
        mUi.progress->setVisible(false);
        mUi.actionStop->setEnabled(false);
        mUi.actionRefresh->setEnabled(true);
        mUi.btnSearch->setEnabled(true);
    };

    mSearch.OnSearchCallback = [=](bool emptyResult) {
        if (emptyResult)
            mUi.btnSearchMore->setText("No more results");
        else mUi.btnSearchMore->setText("Load more results ...");
        mUi.btnSearchMore->setEnabled(!emptyResult);
        const QHeaderView* header = mUi.tableView->horizontalHeader();
        const auto sortColumn = header->sortIndicatorSection();
        const auto sortOrder  = header->sortIndicatorOrder();
        mUi.tableView->sortByColumn(sortColumn, sortOrder);
    };

    QObject::connect(mUi.btnBasic, SIGNAL(clicked()),
        mUi.wBasic, SLOT(show()));
    QObject::connect(mUi.btnBasic, SIGNAL(clicked()),
        mUi.wAdvanced, SLOT(hide()));
    QObject::connect(mUi.btnBasic, SIGNAL(clicked()),
        mUi.wMusic, SLOT(hide()));
    QObject::connect(mUi.btnBasic, SIGNAL(clicked()),
        mUi.wTV, SLOT(hide()));

    QObject::connect(mUi.btnAdvanced, SIGNAL(clicked()),
        mUi.wBasic, SLOT(hide()));
    QObject::connect(mUi.btnAdvanced, SIGNAL(clicked()),
        mUi.wAdvanced, SLOT(show()));
    QObject::connect(mUi.btnAdvanced, SIGNAL(clicked()),
        mUi.wMusic, SLOT(hide()));
    QObject::connect(mUi.btnAdvanced, SIGNAL(clicked()),
        mUi.wTV, SLOT(hide()));

    QObject::connect(mUi.btnTelevision, SIGNAL(clicked()),
        mUi.wBasic, SLOT(hide()));
    QObject::connect(mUi.btnTelevision, SIGNAL(clicked()),
        mUi.wAdvanced, SLOT(hide()));
    QObject::connect(mUi.btnTelevision, SIGNAL(clicked()),
        mUi.wMusic, SLOT(hide()));
    QObject::connect(mUi.btnTelevision, SIGNAL(clicked()),
        mUi.wTV, SLOT(show()));

    QObject::connect(mUi.btnMusic, SIGNAL(clicked()),
        mUi.wBasic, SLOT(hide()));
    QObject::connect(mUi.btnMusic, SIGNAL(clicked()),
        mUi.wAdvanced, SLOT(hide()));
    QObject::connect(mUi.btnMusic, SIGNAL(clicked()),
        mUi.wMusic, SLOT(show()));
    QObject::connect(mUi.btnMusic, SIGNAL(clicked()),
        mUi.wTV, SLOT(hide()));

    auto* model = mUi.tableView->selectionModel();
    QObject::connect(model, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(tableview_selectionChanged()));

    auto* viewport = mUi.tableView->viewport();
    viewport->installEventFilter(this);
    viewport->setMouseTracking(true);

    QObject::connect(&mPopupTimer, SIGNAL(timeout()),
        this, SLOT(popupDetails()));

    DEBUG("Search UI created");
}

Search::~Search()
{
    mSearch.stop();

    DEBUG("Search UI deleted");
}

void Search::addActions(QMenu& menu)
{
    menu.addAction(mUi.actionRefresh);
    menu.addAction(mUi.actionStop);
    menu.addSeparator();
    menu.addAction(mUi.actionDownload);
    menu.addSeparator();
    menu.addAction(mUi.actionSave);
    menu.addAction(mUi.actionOpen);
}

void Search::addActions(QToolBar& bar)
{
    bar.addAction(mUi.actionRefresh);
    bar.addAction(mUi.actionStop);
    bar.addSeparator();
    bar.addAction(mUi.actionDownload);
    bar.addSeparator();
    bar.addAction(mUi.actionSettings);
}

void Search::loadState(app::Settings& settings)
{
    app::loadState("search", mUi.chkMusic, settings);
    app::loadState("search", mUi.chkMovies, settings);
    app::loadState("search", mUi.chkTV, settings);
    app::loadState("search", mUi.chkConsole, settings);
    app::loadState("search", mUi.chkComputer, settings);
    app::loadState("search", mUi.chkAdult, settings);
    app::loadState("search", mUi.editAlbum, settings);
    app::loadState("search", mUi.editTrack, settings);
    app::loadState("search", mUi.editSeason, settings);
    app::loadState("search", mUi.editEpisode, settings);
    app::loadState("search", mUi.editSearch, settings);
    app::loadState("search", mUi.cmbYear, settings);
    app::loadState("search", mUi.cmbIndexer, settings);
    app::loadTableLayout("search", mUi.tableView, settings);

    const auto text = mUi.editSearch->text();
    mUi.editSearch->setFocus();
    mUi.editSearch->setCursorPosition(text.size());
}

void Search::saveState(app::Settings& settings)
{
    app::saveTableLayout("search", mUi.tableView, settings);
    app::saveState("search", mUi.chkMusic, settings);
    app::saveState("search", mUi.chkMovies, settings);
    app::saveState("search", mUi.chkTV, settings);
    app::saveState("search", mUi.chkConsole, settings);
    app::saveState("search", mUi.chkComputer, settings);
    app::saveState("search", mUi.chkAdult, settings);
    app::saveState("search", mUi.editAlbum, settings);
    app::saveState("search", mUi.editTrack, settings);
    app::saveState("search", mUi.editSeason, settings);
    app::saveState("search", mUi.editEpisode, settings);
    app::saveState("search", mUi.editSearch, settings);
    app::saveState("search", mUi.cmbYear, settings);
    app::saveState("search", mUi.cmbIndexer, settings);
}

void Search::updateBackendList(const QStringList& names)
{
    mUi.cmbIndexer->clear();
    for (const auto& name : names)
        mUi.cmbIndexer->addItem(name);
}

void Search::on_actionRefresh_triggered()
{
    beginSearch(0, QuerySize);
}

void Search::on_actionStop_triggered()
{
    mSearch.stop();
    mUi.actionStop->setEnabled(false);
    mUi.progress->setVisible(false);
    mUi.btnSearch->setEnabled(true);
    mUi.actionRefresh->setEnabled(true);
}

void Search::on_actionSettings_triggered()
{
    emit showSettings("Newznab");
}

void Search::on_actionOpen_triggered()
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    for (const auto& index : indices)
    {
        const auto& item = mSearch.getItem(index);
        const auto type  = item.type;
        mSearch.loadItem(index,
            [=](const QByteArray& bytes, const QString& desc) {
                auto* view = new NZBFile(type);
                view->setProperty("parent-object", QVariant::fromValue(static_cast<QObject*>(this)));
                g_win->attachSessionWidget(view);
                view->open(bytes, desc);
            });
    }
    mUi.progress->setVisible(true);
    mUi.actionStop->setEnabled(true);
}

void Search::on_actionSave_triggered()
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    for (const auto& index : indices)
    {
        const auto& item = mSearch.getItem(index);
        const auto& file = g_win->selectNzbSaveFile(item.title + ".nzb");
        if (file.isEmpty())
            continue;
        mSearch.saveItem(index, file);
        mUi.progress->setVisible(true);
        mUi.actionStop->setEnabled(true);
    }
}

void Search::on_actionDownload_triggered()
{
    downloadSelected("");
}

void Search::on_actionDownloadTo_triggered()
{
    const auto& folder = g_win->selectDownloadFolder();
    if (folder.isEmpty())
        return;

    downloadSelected(folder);
}

void Search::on_actionBrowse_triggered()
{
    const auto& folder = g_win->selectDownloadFolder();
    if (folder.isEmpty())
        return;

    downloadSelected(folder);
}

void Search::on_btnSearch_clicked()
{
    beginSearch(0, QuerySize);
}

void Search::on_btnSearchMore_clicked()
{
    // grab the next queryfull of entries.
    beginSearch(mSearchOffset + QuerySize, QuerySize);
}


void Search::on_tableView_customContextMenuRequested(QPoint point)
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
    menu.addAction(mUi.actionStop);
    menu.addSeparator();
    menu.addAction(mUi.actionSettings);
    menu.exec(QCursor::pos());
}

void Search::on_editSearch_returnPressed()
{
    beginSearch(0, QuerySize);
}

void Search::on_editSeason_returnPressed()
{
    beginSearch(0, QuerySize);
}

void Search::on_editEpisode_returnPressed()
{
    beginSearch(0, QuerySize);
}

void Search::on_editAlbum_returnPressed()
{
    beginSearch(0, QuerySize);
}

void Search::on_editTrack_returnPressed()
{
    beginSearch(0, QuerySize);
}

void Search::on_lblWildcard_linkActivated(QString)
{
    DlgWildcards dlg(this);
    dlg.exec();
}

void Search::tableview_selectionChanged()
{
    const auto indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    mUi.actionDownload->setEnabled(true);
    mUi.actionSave->setEnabled(true);
    mUi.actionOpen->setEnabled(true);
}

void Search::downloadToPrevious()
{
    const auto* action = qobject_cast<const QAction*>(sender());

    const auto folder = action->text();

    downloadSelected(folder);
}
void Search::popupDetails()
{
    mPopupTimer.stop();
    if (!DlgMovie::isLookupEnabled())
        return;

    if (mMovieDetails && mMovieDetails->isVisible())
        return;

    if (!mUi.tableView->underMouse())
        return;

    const auto global = QCursor::pos();
    const auto local  = mUi.tableView->viewport()->mapFromGlobal(global);

    // see if the current item under the mouse pointer is also
    // currently selected.
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

    using media = app::MediaType;

    const auto& item = mSearch.getItem(sel);
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

bool Search::eventFilter(QObject* object, QEvent* event)
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
    return QObject::eventFilter(object, event);
}

void Search::downloadSelected(const QString& folder)
{
    const auto& indices = mUi.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    bool actions = false;

    for (const auto& index : indices)
    {
        const auto& item = mSearch.getItem(index);
        const auto& desc = item.title;

        if (!passDuplicateCheck(this, desc, item.type))
            continue;

        if (!passSpaceCheck(this, desc, folder, item.size, item.size, item.type))
            continue;

        const auto acc = selectAccount(this, desc);
        if (acc == 0)
            continue;

        mSearch.downloadItem(index, folder, acc);
        actions = true;
    }
    if (actions)
    {
        mUi.progress->setVisible(true);
        mUi.actionStop->setEnabled(true);
    }
}

void Search::beginSearch(quint32 queryOffset, quint32 querySize)
{
    const auto index = mUi.cmbIndexer->currentIndex();
    if (index == -1)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setIcon(QMessageBox::Information);
        msg.setText(tr("It looks like you don't have any Usenet search engines configured.\n"
            "Would you like to configure sites now?"));
        if (msg.exec() == QMessageBox::No)
            return;

        emit showSettings("Newznab");
        return;
    }
    const auto& name = mUi.cmbIndexer->currentText();

    if (queryOffset == 0)
        mSearch.clear();

    mSearchOffset = queryOffset;

    // create the newznab search engine.
    // which is then given to the model as an app::Indexer.
    auto engine = mNewznab.makeSearchEngine(name);

    if (mUi.btnBasic->isChecked())
    {
        app::Search::Basic query;
        query.keywords = mUi.editSearch->text();
        query.qoffset  = queryOffset;
        query.qsize    = querySize;
        mSearch.beginSearch(query, std::move(engine));
    }
    else if (mUi.btnAdvanced->isChecked())
    {
        app::Search::Advanced query;
        query.music      = mUi.chkMusic->isChecked();
        query.movies     = mUi.chkMovies->isChecked();
        query.television = mUi.chkTV->isChecked();
        query.console    = mUi.chkConsole->isChecked();
        query.computer   = mUi.chkComputer->isChecked();
        query.adult      = mUi.chkAdult->isChecked();
        query.keywords   = mUi.editSearch->text();
        query.qoffset    = queryOffset;
        query.qsize      = querySize;
        mSearch.beginSearch(query, std::move(engine));
    }
    else if (mUi.btnMusic->isChecked())
    {
        app::Search::Music music;
        music.keywords = mUi.editSearch->text();
        music.album    = mUi.editAlbum->text();
        music.track    = mUi.editTrack->text();
        music.year     = mUi.cmbYear->currentText();
        music.qoffset  = queryOffset;
        music.qsize    = querySize;
        mSearch.beginSearch(music, std::move(engine));

    }
    else if (mUi.btnTelevision->isChecked())
    {
        app::Search::Television tv;
        tv.episode  = mUi.editEpisode->text();
        tv.season   = mUi.editSeason->text();
        tv.keywords = mUi.editSearch->text();
        tv.qoffset  = queryOffset;
        tv.qsize    = querySize;
        mSearch.beginSearch(tv, std::move(engine));
    }

    mUi.progress->setVisible(true);
    mUi.actionStop->setEnabled(true);
    mUi.actionDownload->setEnabled(false);
    mUi.actionOpen->setEnabled(false);
    mUi.actionSave->setEnabled(false);
    mUi.actionRefresh->setEnabled(false);
    mUi.btnSearchMore->setEnabled(false);
    mUi.btnSearch->setEnabled(false);
}

void Search::startMovieDownload(const QString& guid)
{
    downloadSelected("");
}


} // gui
