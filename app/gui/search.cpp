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
#  include <QDate>
#include <newsflash/warnpop.h>
#include "mainwindow.h"
#include "search.h"
#include "nzbfile.h"
#include "../debug.h"
#include "../search.h"
#include "../settings.h"
#include "../utility.h"
#include "../engine.h"

#include "../newznab.h"

namespace gui
{

Search::Search() 
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
    
    const auto thisYear = QDate::currentDate().year();
    for (int year=thisYear; year>=1900; --year)
        ui_.cmbYear->addItem(QString::number(year));

    model_.OnReadyCallback = [=]() {
        ui_.progress->setVisible(false);
        ui_.actionStop->setEnabled(false);
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

void Search::loadState(app::Settings& settings)
{
    app::loadTableLayout("search", ui_.tableView, settings);
}

void Search::saveState(app::Settings& settings)
{
    app::saveTableLayout("search", ui_.tableView, settings);    
}

void Search::on_actionRefresh_triggered()
{
    on_btnSearch_clicked();
}

void Search::on_actionStop_triggered()
{
    model_.stop();
    ui_.actionStop->setEnabled(false);
    ui_.progress->setVisible(false);
}

void Search::on_actionSettings_triggered()
{

}

void Search::on_actionOpen_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    for (const auto& index : indices)
    {
        model_.loadItem(index, 
            [](const QByteArray& bytes, const QString& desc) {
                auto* view = new NZBFile();
                g_win->attach(view, false);
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

    }
    ui_.progress->setVisible(true);
    ui_.actionStop->setEnabled(true);    
}

void Search::on_btnSearch_clicked()
{
    std::unique_ptr<app::Newznab> nab(new app::Newznab);
    app::Newznab::Account a;
    a.apiurl = "http://nzbs.org/api/";
    a.apikey = "debb752d0452ebf5174500aa19844f8e";    
    nab->setAccount(a);

    if (ui_.btnBasic->isChecked())
    {
        app::Search::Basic query;
        query.keywords = ui_.editSearch->text();
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
        model_.beginSearch(query, std::move(nab));
    }
    else if (ui_.btnMusic->isChecked())
    {
        app::Search::Music music;
        music.keywords = ui_.editSearch->text();        
        music.album    = ui_.editAlbum->text();
        music.track    = ui_.editTrack->text();
        music.year     = ui_.cmbYear->currentText();
        model_.beginSearch(music, std::move(nab));

    }
    else if (ui_.btnTelevision->isChecked())
    {
        app::Search::Television tv;
        tv.episode  = ui_.editEpisode->text();
        tv.season   = ui_.editSeason->text();
        tv.keywords = ui_.editSearch->text();
        model_.beginSearch(tv, std::move(nab));
    }

    ui_.progress->setVisible(true);
    ui_.actionStop->setEnabled(true);
    ui_.actionDownload->setEnabled(false);
    ui_.actionOpen->setEnabled(false);
    ui_.actionSave->setEnabled(false);
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


void Search::downloadSelected(const QString& folder)
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    auto callback = [=](const QByteArray& buff, const QString& desc, const QString& path, quint32 acc) {
        app::g_engine->downloadNzbContents(acc, path, desc, buff);
    };

    for (const auto& index : indices)
    {
        const auto& item = model_.getItem(index);
        const auto& desc = item.title;
        const auto acc   = g_win->chooseAccount(desc);
        if (acc == 0)
            continue;

        model_.loadItem(index,
            std::bind(callback, std::placeholders::_1, std::placeholders::_2,
                folder, acc));

    }
    ui_.progress->setVisible(true);
    ui_.actionStop->setEnabled(true);
}


} // gui