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

#define LOGTAG "news"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMessageBox>
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#  include <QtGui/QPixmap>
#  include <QtGui/QMovie>
#  include <QFile>
#  include <QFileInfo>
#include <newsflash/warnpop.h>
#include "newslist.h"
#include "newsgroup.h"
#include "mainwindow.h"
#include "../accounts.h"
#include "../homedir.h"
#include "../settings.h"
#include "../debug.h"
#include "../format.h"
#include "../utility.h"

namespace gui
{

NewsList::NewsList() : curAccount_(0)
{
    ui_.setupUi(this);
    ui_.tableGroups->setModel(&model_);
    ui_.tableGroups->setColumnWidth(0, 150);
    ui_.progressBar->setMaximum(0);
    ui_.progressBar->setMinimum(0);
    ui_.progressBar->setValue(0);
    ui_.progressBar->setVisible(false);
    ui_.actionRefresh->setShortcut(QKeySequence::Refresh);
    ui_.actionRefresh->setEnabled(false);    
    ui_.actionFavorite->setEnabled(false);
    ui_.actionStop->setEnabled(false);    

    const auto nameWidth = ui_.tableGroups->columnWidth((int)app::NewsList::Columns::name);
    const auto subsWidth = ui_.tableGroups->columnWidth((int)app::NewsList::Columns::subscribed);
    ui_.tableGroups->setColumnWidth((int)app::NewsList::Columns::name, nameWidth * 3);
    ui_.tableGroups->setColumnWidth((int)app::NewsList::Columns::subscribed, 32);

    QObject::connect(app::g_accounts, SIGNAL(accountsUpdated()),
        this, SLOT(accountsUpdated()));

    QObject::connect(&model_, SIGNAL(progressUpdated(quint32, quint32, quint32)),
        this, SLOT(progressUpdated(quint32, quint32, quint32)));
    QObject::connect(&model_, SIGNAL(loadComplete(quint32)),
        this, SLOT(loadComplete(quint32)));
    QObject::connect(&model_, SIGNAL(makeComplete(quint32)),
        this, SLOT(makeComplete(quint32)));
}

NewsList::~NewsList()
{}

void NewsList::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();
    menu.addAction(ui_.actionBrowse);
    menu.addAction(ui_.actionFavorite);
    menu.addAction(ui_.actionUnfavorite);
    menu.addSeparator();
    menu.addAction(ui_.actionStop);
}

void NewsList::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionRefresh);
    bar.addSeparator();
    bar.addAction(ui_.actionBrowse);
    bar.addAction(ui_.actionFavorite);
    bar.addAction(ui_.actionUnfavorite);
    bar.addSeparator();
    bar.addAction(ui_.actionStop);
}

void NewsList::activate(QWidget*)
{
    on_cmbAccounts_currentIndexChanged();
}

MainWidget::info NewsList::getInformation() const
{
    return {"news.html", true};
}

void NewsList::loadState(app::Settings& settings)
{
    app::loadState("newslist", ui_.chkFavorites, settings);
    app::loadTableLayout("newslist", ui_.tableGroups, settings);

    accountsUpdated();
}

void NewsList::saveState(app::Settings& settings)
{
    app::saveState("newslist", ui_.chkFavorites, settings);
    app::saveTableLayout("newslist", ui_.tableGroups, settings);
}

void NewsList::on_actionBrowse_triggered()
{
    const auto* account = app::g_accounts->findAccount(curAccount_);
    Q_ASSERT(account);

    const auto& indices = ui_.tableGroups->selectionModel()->selectedRows();
    for (const auto& i : indices)
    {
        const auto& datapath = account->datapath;
        const auto& group    = model_.getName(i);
        const auto account   = curAccount_;

        auto* news = new NewsGroup(account, datapath, group);
        news->load();
        g_win->attach(news, false, true);
    }
}


void NewsList::on_actionRefresh_triggered()
{
    model_.clear();

    const auto* account = app::g_accounts->findAccount(curAccount_);
    Q_ASSERT(account);

    DEBUG("Refresh newslist '%1' (%2)", account->name, account->id);

    const auto file = app::homedir::file(account->name + ".lst");

    ui_.progressBar->setMaximum(0);        
    ui_.progressBar->setVisible(true);
    model_.makeListing(file, account->id);        

}

void NewsList::on_actionFavorite_triggered()
{
    auto indices = ui_.tableGroups->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    Q_ASSERT(curAccount_);

    model_.subscribe(indices, curAccount_);
}

void NewsList::on_actionUnfavorite_triggered()
{
    auto indices = ui_.tableGroups->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    Q_ASSERT(curAccount_);

    QMessageBox msg(this);
    msg.setIcon(QMessageBox::Question);
    msg.setWindowTitle(tr("Delete Group Data"));
    msg.setText(tr("Would you like to remove the local database?"));

    model_.unsubscribe(indices, curAccount_);
    if (ui_.chkFavorites->isChecked())
        model_.filter(true);
}


void NewsList::on_cmbAccounts_currentIndexChanged()
{
    const auto index = ui_.cmbAccounts->currentIndex();
    if (index == -1)
        return;

    const auto name = ui_.cmbAccounts->currentText();
    const auto file = app::homedir::file(name + ".lst");

    const auto numAccounts = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<numAccounts; ++i)
    {
        const auto& acc = app::g_accounts->getAccount(i);
        if (acc.name != name)
            continue;
        if (curAccount_ == acc.id)
            return;

        curAccount_ = acc.id;

        DEBUG("Current account changed to '%1' (%2)", acc.name, acc.id);

        ui_.progressBar->setMaximum(0);
        ui_.progressBar->setVisible(true);

        QFileInfo info(file);
        if (info.exists())
        {
            model_.clear();
            model_.loadListing(file, acc.id);
        }
        else
        {
            model_.clear();
            model_.makeListing(file, acc.id);
        }
        return;
    }
    Q_ASSERT(!"account was not found");
}

void NewsList::on_tableGroups_customContextMenuRequested(QPoint point)
{
    QMenu menu(this);
    menu.addAction(ui_.actionBrowse);
    menu.addSeparator();
    menu.addAction(ui_.actionFavorite);
    menu.addAction(ui_.actionUnfavorite);
    menu.exec(QCursor::pos());
}

void NewsList::on_chkFavorites_clicked(bool state)
{
    model_.filter(state);
}


void NewsList::accountsUpdated()
{
    ui_.cmbAccounts->clear();
    ui_.cmbAccounts->blockSignals(true);

    int currentAccountIndex = -1;

    const auto numAccounts = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<numAccounts; ++i)
    {
        const auto& account = app::g_accounts->getAccount(i);
        if (account.id == curAccount_)
            currentAccountIndex = (int)i;

        ui_.cmbAccounts->addItem(account.name);
    }

    if (currentAccountIndex == -1)
    {
        model_.clear();
        curAccount_ = 0;
    }
    else
    {
        ui_.cmbAccounts->setCurrentIndex(currentAccountIndex);
    }

    ui_.cmbAccounts->blockSignals(false);    

    ui_.actionRefresh->setEnabled(numAccounts != 0);
    ui_.actionFavorite->setEnabled(numAccounts != 0);
}

void NewsList::progressUpdated(quint32 acc, quint32 maxValue, quint32 curValue)
{
    if (curAccount_ != acc)
        return;

    ui_.progressBar->setMaximum(maxValue);
    ui_.progressBar->setValue(curValue);
}

void NewsList::loadComplete(quint32 acc)
{
    DEBUG("Load complete %1", acc);

    if (curAccount_ != acc)
        return;

    ui_.progressBar->setVisible(false);

    resort();
}

void NewsList::makeComplete(quint32 accountId)
{
    DEBUG("Make complete %1", accountId);

    if (curAccount_ != accountId)
        return;

    ui_.progressBar->setVisible(false);

    const auto numAcccounts = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<numAcccounts; ++i)
    {
        const auto& acc = app::g_accounts->getAccount(i);
        if (acc.id != accountId)
            continue;

        const auto& name = acc.name;
        const auto& file = app::homedir::file(name + ".lst");
        model_.loadListing(file, acc.id);
        return;
    }

    Q_ASSERT(!"Account was not found");
}

void NewsList::resort()
{
    const QHeaderView* header = ui_.tableGroups->horizontalHeader();
    const auto sortColumn = header->sortIndicatorSection();
    const auto sortOrder  = header->sortIndicatorOrder();
    ui_.tableGroups->sortByColumn(sortColumn, sortOrder);
}

} // gui