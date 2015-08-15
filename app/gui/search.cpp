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
#  include <QtGui/QMessageBox>
#  include <QDate>
#include <newsflash/warnpop.h>
#include "mainwindow.h"
#include "search.h"
#include "searchmodule.h"
#include "nzbfile.h"
#include "dlgmovie.h"
#include "dlgwildcards.h"
#include "common.h"
#include "../debug.h"
#include "../search.h"
#include "../settings.h"
#include "../utility.h"
#include "../engine.h"

namespace gui
{

const auto QuerySize = 100;

Search::Search(SearchModule& module) : module_(module) 
{
    ui_.setupUi(this);
    ui_.btnBasic->setChecked(true);
    ui_.wBasic->setVisible(true);
    ui_.wAdvanced->setVisible(false);
    ui_.wTV->setVisible(false);
    ui_.wMusic->setVisible(false);
    ui_.tableView->setModel(model_.getModel());
    ui_.progress->setVisible(false);
    ui_.progress->setMinimum(0);
    ui_.progress->setMaximum(0);
    ui_.actionStop->setEnabled(false);
    ui_.actionDownload->setEnabled(false);
    ui_.actionSave->setEnabled(false);
    ui_.actionOpen->setEnabled(false);
    ui_.editSearch->setFocus();
    ui_.btnSearchMore->setEnabled(false);
    ui_.editSearch->setFocus();

    ui_.cmbYear->addItem("");
    const auto thisYear = QDate::currentDate().year();
    for (int year=thisYear; year>=1900; --year)
        ui_.cmbYear->addItem(QString::number(year));

    model_.OnReadyCallback = [=]() {
        ui_.progress->setVisible(false);
        ui_.actionStop->setEnabled(false);
    };

    model_.OnSearchCallback = [=](bool emptyResult) {
        if (emptyResult)
            ui_.btnSearchMore->setText("No more results");
        else ui_.btnSearchMore->setText("Load more results ...");
        ui_.btnSearchMore->setEnabled(!emptyResult);
        const QHeaderView* header = ui_.tableView->horizontalHeader();
        const auto sortColumn = header->sortIndicatorSection();
        const auto sortOrder  = header->sortIndicatorOrder();
        ui_.tableView->sortByColumn(sortColumn, sortOrder);
    };

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

    auto* model = ui_.tableView->selectionModel();
    QObject::connect(model, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(tableview_selectionChanged()));

    auto* viewport = ui_.tableView->viewport();
    viewport->installEventFilter(this);
    viewport->setMouseTracking(true);

    QObject::connect(&popup_, SIGNAL(timeout()),
        this, SLOT(popupDetails()));

    DEBUG("Search UI created");
}

Search::~Search()
{
    DEBUG("Search UI deleted");
}

void Search::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();
    menu.addAction(ui_.actionDownload);
    menu.addSeparator();
    menu.addAction(ui_.actionSave);
    menu.addAction(ui_.actionOpen);
    menu.addSeparator();
    menu.addAction(ui_.actionStop); 
}

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

void Search::loadState(app::Settings& settings)
{
    app::loadState("search", ui_.chkMusic, settings);
    app::loadState("search", ui_.chkMovies, settings);
    app::loadState("search", ui_.chkTV, settings);
    app::loadState("search", ui_.chkConsole, settings);
    app::loadState("search", ui_.chkComputer, settings);
    app::loadState("search", ui_.chkXXX, settings);
    app::loadState("search", ui_.editAlbum, settings);
    app::loadState("search", ui_.editTrack, settings);
    app::loadState("search", ui_.editSeason, settings);
    app::loadState("search", ui_.editEpisode, settings);
    app::loadState("search", ui_.editSearch, settings);
    app::loadState("search", ui_.cmbYear, settings);
    app::loadTableLayout("search", ui_.tableView, settings);

    const auto text = ui_.editSearch->text();
    ui_.editSearch->setFocus();
    ui_.editSearch->setCursorPosition(text.size());
}

void Search::saveState(app::Settings& settings)
{
    app::saveTableLayout("search", ui_.tableView, settings);    
    app::saveState("search", ui_.chkMusic, settings);
    app::saveState("search", ui_.chkMovies, settings);
    app::saveState("search", ui_.chkTV, settings);
    app::saveState("search", ui_.chkConsole, settings);
    app::saveState("search", ui_.chkComputer, settings);
    app::saveState("search", ui_.chkXXX, settings);
    app::saveState("search", ui_.editAlbum, settings);
    app::saveState("search", ui_.editTrack, settings);
    app::saveState("search", ui_.editSeason, settings);
    app::saveState("search", ui_.editEpisode, settings);
    app::saveState("search", ui_.editSearch, settings);    
    app::saveState("search", ui_.cmbYear, settings);
}

void Search::updateBackendList(const QStringList& names)
{
    ui_.cmbIndexer->clear();
    for (const auto& name : names)
        ui_.cmbIndexer->addItem(name);
}

void Search::on_actionRefresh_triggered()
{
    beginSearch(0, QuerySize);
}

void Search::on_actionStop_triggered()
{
    model_.stop();
    ui_.actionStop->setEnabled(false);
    ui_.progress->setVisible(false);
}

void Search::on_actionSettings_triggered()
{
    emit showSettings(this);
}

void Search::on_actionOpen_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    for (const auto& index : indices)
    {
        model_.loadItem(index, 
            [=](const QByteArray& bytes, const QString& desc) {
                auto* view = new NZBFile();
                view->setProperty("parent-object", QVariant::fromValue(static_cast<QObject*>(this)));
                g_win->attach(view, false, true);
                view->open(bytes, desc);
            });
    }
    ui_.progress->setVisible(true);
    ui_.actionStop->setEnabled(true);
}

void Search::on_actionSave_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    for (const auto& index : indices)
    {
        const auto& item = model_.getItem(index);
        const auto& file = g_win->selectNzbSaveFile(item.title + ".nzb");
        if (file.isEmpty())
            continue;
        model_.saveItem(index, file);
        ui_.progress->setVisible(true);
        ui_.actionStop->setEnabled(true);    
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
    beginSearch(offset_ + QuerySize, QuerySize);
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
    const auto indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    ui_.actionDownload->setEnabled(true);
    ui_.actionSave->setEnabled(true);
    ui_.actionOpen->setEnabled(true);
}

void Search::downloadToPrevious()
{
    const auto* action = qobject_cast<const QAction*>(sender());

    const auto folder = action->text();

    downloadSelected(folder);
}
void Search::popupDetails()
{
    popup_.stop();
    if (movie_ && movie_->isVisible())
        return;

    if (!ui_.tableView->underMouse())
        return;

    const auto global = QCursor::pos();
    const auto local  = ui_.tableView->viewport()->mapFromGlobal(global);

    // see if the current item under the mouse pointer is also
    // currently selected. 
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

    using media = app::MediaType;

    const auto& item = model_.getItem(sel);
    if (isMovie(item.type))
    {
        const auto& title = app::findMovieTitle(item.title);
        if (title.isEmpty())
            return;
        if (!movie_)
            movie_.reset(new DlgMovie(this));
        movie_->lookupMovie(title);
    }
    else if (isTVSeries(item.type))
    {
        const auto& title = app::findTVSeriesTitle(item.title);
        if (title.isEmpty())
            return;

        if (!movie_)
            movie_.reset(new DlgMovie(this));

        movie_->lookupSeries(title);
    }
}

bool Search::eventFilter(QObject* object, QEvent* event)
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
    return QObject::eventFilter(object, event);
}

void Search::downloadSelected(const QString& folder)
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    for (const auto& index : indices)
    {
        const auto& item = model_.getItem(index);
        const auto& desc = item.title;
        const auto acc   = selectAccount(this, desc);
        if (acc == 0)
            continue;

        model_.downloadItem(index, folder, acc);
    }
    ui_.progress->setVisible(true);
    ui_.actionStop->setEnabled(true);
}

void Search::beginSearch(quint32 queryOffset, quint32 querySize)
{
    const auto index = ui_.cmbIndexer->currentIndex();
    if (index == -1)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setIcon(QMessageBox::Information);
        msg.setText(tr("It looks like you don't have any Usenet search engines configured.\n"
            "Would you like to configure sites now?"));
        if (msg.exec() == QMessageBox::No)
            return;

        emit showSettings(this);
        return;
    }
    const auto& name    = ui_.cmbIndexer->currentText();
    const auto& account = module_.getAccount(name);

    if (queryOffset == 0)
        model_.clear();

    offset_ = queryOffset;

    std::unique_ptr<app::Newznab> nab(new app::Newznab);
    nab->setAccount(account);

    if (ui_.btnBasic->isChecked())
    {
        app::Search::Basic query;
        query.keywords = ui_.editSearch->text();
        query.qoffset  = queryOffset;
        query.qsize    = querySize;
        model_.beginSearch(query, std::move(nab));
    }
    else if (ui_.btnAdvanced->isChecked())
    {
        app::Search::Advanced query;
        query.music      = ui_.chkMusic->isChecked();
        query.movies     = ui_.chkMovies->isChecked();
        query.television = ui_.chkTV->isChecked();
        query.console    = ui_.chkConsole->isChecked();
        query.computer   = ui_.chkComputer->isChecked();
        query.porno      = ui_.chkXXX->isChecked();
        query.keywords   = ui_.editSearch->text();
        query.qoffset    = queryOffset;        
        query.qsize      = querySize;
        model_.beginSearch(query, std::move(nab));
    }
    else if (ui_.btnMusic->isChecked())
    {
        app::Search::Music music;
        music.keywords = ui_.editSearch->text();        
        music.album    = ui_.editAlbum->text();
        music.track    = ui_.editTrack->text();
        music.year     = ui_.cmbYear->currentText();
        music.qoffset  = queryOffset;        
        music.qsize    = querySize;
        model_.beginSearch(music, std::move(nab));

    }
    else if (ui_.btnTelevision->isChecked())
    {
        app::Search::Television tv;
        tv.episode  = ui_.editEpisode->text();
        tv.season   = ui_.editSeason->text();
        tv.keywords = ui_.editSearch->text();
        tv.qoffset  = queryOffset;        
        tv.qsize    = querySize;
        model_.beginSearch(tv, std::move(nab));
    }

    ui_.progress->setVisible(true);
    ui_.actionStop->setEnabled(true);
    ui_.actionDownload->setEnabled(false);
    ui_.actionOpen->setEnabled(false);
    ui_.actionSave->setEnabled(false);    
}



} // gui