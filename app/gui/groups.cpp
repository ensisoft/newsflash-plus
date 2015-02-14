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

#include "groups.h"
#include "newsgroup.h"
#include "mainwindow.h"
#include "../accounts.h"
#include "../homedir.h"
#include "../settings.h"
#include "../debug.h"
#include "../format.h"

using app::str;

namespace gui
{

Groups::Groups() : curAccount_(0)
{
    ui_.setupUi(this);
    ui_.tableGroups->setModel(&model_);
    ui_.tableGroups->setColumnWidth(0, 150);
    ui_.progressBar->setMaximum(0);
    ui_.progressBar->setMinimum(0);
    ui_.progressBar->setValue(0);
    ui_.progressBar->setVisible(false);

    ui_.findContainer->setVisible(false);

    const auto nameWidth = ui_.tableGroups->columnWidth((int)app::NewsList::Columns::name);
    const auto subsWidth = ui_.tableGroups->columnWidth((int)app::NewsList::Columns::subscribed);
    ui_.tableGroups->setColumnWidth((int)app::NewsList::Columns::name, nameWidth * 3);
    ui_.tableGroups->setColumnWidth((int)app::NewsList::Columns::subscribed, 32);

    ui_.actionRefresh->setShortcut(QKeySequence::Refresh);
    ui_.actionRefresh->setEnabled(false);

    ui_.actionFind->setShortcut(QKeySequence::Find);

    ui_.actionFavorite->setEnabled(false);

    ui_.actionStop->setEnabled(false);

    QObject::connect(app::g_accounts, SIGNAL(accountsUpdated()),
        this, SLOT(accountsUpdated()));

    QObject::connect(&model_, SIGNAL(progressUpdated(quint32, quint32, quint32)),
        this, SLOT(progressUpdated(quint32, quint32, quint32)));
    QObject::connect(&model_, SIGNAL(loadComplete(quint32)),
        this, SLOT(loadComplete(quint32)));
    QObject::connect(&model_, SIGNAL(makeComplete(quint32)),
        this, SLOT(makeComplete(quint32)));
}

Groups::~Groups()
{}

void Groups::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();
    menu.addAction(ui_.actionFind);
    menu.addSeparator();
    menu.addAction(ui_.actionBrowse);
    menu.addAction(ui_.actionFavorite);
    menu.addAction(ui_.actionUnfavorite);
    menu.addSeparator();
    menu.addAction(ui_.actionStop);
}

void Groups::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionRefresh);
    bar.addSeparator();
    bar.addAction(ui_.actionFind);
    bar.addSeparator();
    bar.addAction(ui_.actionBrowse);
    bar.addAction(ui_.actionFavorite);
    bar.addAction(ui_.actionUnfavorite);
    bar.addSeparator();
    bar.addAction(ui_.actionStop);
}

void Groups::activate(QWidget*)
{
    on_cmbAccounts_currentIndexChanged();
}

MainWidget::info Groups::getInformation() const
{
    return {"news.html", true, true};
}

void Groups::loadState(app::Settings& settings)
{
    const auto favorites  = settings.get("news", "show_favorites_only", false);
    const auto sortColumn = settings.get("news", "sort_column", (int)app::NewsList::Columns::name);
    const auto sortOrder  = settings.get("news", "sort_order", (int)Qt::AscendingOrder);

    ui_.chkFavorites->setChecked(favorites);
    ui_.tableGroups->sortByColumn(sortColumn, (Qt::SortOrder)sortOrder);

    const auto* model = ui_.tableGroups->model();
    for (int i=0; i<model->columnCount() - 1; ++i)
    {
        const auto name  = QString("table_col_%1_width").arg(i);
        const auto width = settings.get("news", name, ui_.tableGroups->columnWidth(i));
        ui_.tableGroups->setColumnWidth(i, width);
    }

    accountsUpdated();
}

bool Groups::saveState(app::Settings& settings)
{
    const auto favorites = ui_.chkFavorites->isChecked();
    const QHeaderView* header = ui_.tableGroups->horizontalHeader();
    const auto sortColumn = header->sortIndicatorSection();
    const auto sortOrder  = header->sortIndicatorOrder();

    settings.set("news", "show_favorites_only", favorites);
    settings.set("news", "sort_column", sortColumn);
    settings.set("news", "sort_order", sortOrder);

    const auto* model = ui_.tableGroups->model();
    for (int i=0; i<model->columnCount() - 1; ++i)
    {
        const auto name  = QString("table_col_%1_width").arg(i);
        const auto width = ui_.tableGroups->columnWidth(i);
        settings.set("news", name, width);
    }

    return true;
}

void Groups::on_actionBrowse_triggered()
{
    const auto& indices = ui_.tableGroups->selectionModel()->selectedRows();

    for (int i=0; i<indices.size(); ++i)
    {
        auto* news = new NewsGroup();
        g_win->attach(news, true);
    }
}

void Groups::on_actionFind_triggered()
{
    ui_.findContainer->setVisible(true);
    ui_.editFind->setFocus();
}

void Groups::on_actionRefresh_triggered()
{
    model_.clear();

    Q_ASSERT(curAccount_);

    const auto numAccounts = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<numAccounts; ++i)
    {
        const auto& acc = app::g_accounts->getAccount(i);
        if (curAccount_ != acc.id)
            continue;

        DEBUG(str("Refresh newslist '_1' (_2)", acc.name, acc.id));

        const auto file = app::homedir::file(acc.name + ".lst");

        ui_.progressBar->setMaximum(0);        
        ui_.progressBar->setVisible(true);
        model_.makeListing(file, acc.id);        
        return;
    }
    Q_ASSERT(!"account was not found");
}

void Groups::on_actionFavorite_triggered()
{
    auto indices = ui_.tableGroups->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    Q_ASSERT(curAccount_);

    model_.subscribe(indices, curAccount_);
}

void Groups::on_actionUnfavorite_triggered()
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
        resort();
}

void Groups::on_btnCloseFind_clicked()
{
    ui_.findContainer->setVisible(false);
}

void Groups::on_cmbAccounts_currentIndexChanged()
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

        DEBUG(str("Current account changed to '_1' (_2)", acc.name, acc.id));

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

void Groups::on_tableGroups_customContextMenuRequested(QPoint point)
{
    QMenu menu(this);
    menu.addAction(ui_.actionBrowse);
    menu.addSeparator();
    menu.addAction(ui_.actionFavorite);
    menu.addAction(ui_.actionUnfavorite);
    menu.exec(QCursor::pos());
}

void Groups::on_chkFavorites_clicked(bool state)
{
    resort();
}

void Groups::on_editFind_returnPressed()
{
    resort();
}

void Groups::on_editFind_textChanged()
{
    //resort();
}

void Groups::accountsUpdated()
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

void Groups::progressUpdated(quint32 acc, quint32 maxValue, quint32 curValue)
{
    if (curAccount_ != acc)
        return;

    //DEBUG(str("Progress update max _1 cur _2", maxValue, curValue));

    ui_.progressBar->setMaximum(maxValue);
    ui_.progressBar->setValue(curValue);
}

void Groups::loadComplete(quint32 acc)
{
    DEBUG(str("Load complete _1", acc));

    if (curAccount_ != acc)
        return;

    ui_.progressBar->setVisible(false);

    resort();
}

void Groups::makeComplete(quint32 accountId)
{
    DEBUG(str("Make complete _1", accountId));

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

void Groups::resort()
{
    QString filter;
    if (ui_.editFind->isVisible())
        filter = ui_.editFind->text();

    bool favorites = ui_.chkFavorites->isChecked();

    model_.filter(filter, favorites);

    const auto numRows = model_.rowCount(QModelIndex());
    if (numRows == 0)
        ui_.lblFind->setText(tr("No matches"));
    else ui_.lblFind->setText(tr("%1 matches").arg(numRows));

    ui_.actionFavorite->setEnabled(numRows != 0);

    const QHeaderView* header = ui_.tableGroups->horizontalHeader();
    const auto sortColumn = header->sortIndicatorSection();
    const auto sortOrder  = header->sortIndicatorOrder();
    ui_.tableGroups->sortByColumn(sortColumn, sortOrder);
}

} // gui