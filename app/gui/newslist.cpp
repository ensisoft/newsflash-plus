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

NewsList::NewsList() : m_curAccount(0)
{
    m_ui.setupUi(this);
    m_ui.tableGroups->setModel(&m_model);
    m_ui.tableGroups->setColumnWidth(0, 150);
    m_ui.progressBar->setMaximum(0);
    m_ui.progressBar->setMinimum(0);
    m_ui.progressBar->setValue(0);
    m_ui.progressBar->setVisible(false);
    m_ui.actionRefresh->setShortcut(QKeySequence::Refresh);
    m_ui.actionRefresh->setEnabled(false);    
    m_ui.actionFavorite->setEnabled(false);
    m_ui.actionStop->setEnabled(false);    
    m_ui.chkShowEmpty->setChecked(false);

    const auto nameWidth = m_ui.tableGroups->columnWidth((int)app::NewsList::Columns::Name);
    m_ui.tableGroups->setColumnWidth((int)app::NewsList::Columns::Name, nameWidth * 3);
    m_ui.tableGroups->setColumnWidth((int)app::NewsList::Columns::Subscribed, 32);

    QObject::connect(app::g_accounts, SIGNAL(accountsUpdated()),
        this, SLOT(accountsUpdated()));

    QObject::connect(&m_model, SIGNAL(progressUpdated(quint32, quint32, quint32)),
        this, SLOT(progressUpdated(quint32, quint32, quint32)));
    QObject::connect(&m_model, SIGNAL(loadComplete(quint32)),
        this, SLOT(loadComplete(quint32)));
    QObject::connect(&m_model, SIGNAL(makeComplete(quint32)),
        this, SLOT(makeComplete(quint32)));
}

NewsList::~NewsList()
{}

void NewsList::addActions(QMenu& menu)
{
    menu.addAction(m_ui.actionRefresh);
    menu.addSeparator();
    menu.addAction(m_ui.actionBrowse);
    menu.addAction(m_ui.actionFavorite);
    menu.addAction(m_ui.actionUnfavorite);
    menu.addSeparator();
    menu.addAction(m_ui.actionDeleteData);
    menu.addSeparator();
    menu.addAction(m_ui.actionStop);
}

void NewsList::addActions(QToolBar& bar)
{
    bar.addAction(m_ui.actionRefresh);
    bar.addSeparator();
    bar.addAction(m_ui.actionBrowse);
    bar.addAction(m_ui.actionFavorite);
    bar.addAction(m_ui.actionUnfavorite);
    bar.addSeparator();
    bar.addAction(m_ui.actionDeleteData);
    bar.addSeparator();
    bar.addAction(m_ui.actionStop);
}

void NewsList::activate(QWidget*)
{
    on_cmbAccounts_currentIndexChanged();
}

MainWidget::info NewsList::getInformation() const
{
    return {"news.html", true};
}

Finder* NewsList::getFinder() 
{
    return this;
}

bool NewsList::isMatch(const QString& str, std::size_t index, bool caseSensitive)
{
    const auto& name = m_model.getName(index);
    if (!caseSensitive)
    {
        auto upper = name.toUpper();
        if (upper.indexOf(str) != -1)
            return true;
    }
    else
    {
        if (name.indexOf(str) != -1)
            return true;
    }
    return false;
}

bool NewsList::isMatch(const QRegExp& regex, std::size_t index)
{
    const auto& name = m_model.getName(index);
    if (regex.indexIn(name) != -1)
        return true;
    return false;
}

std::size_t NewsList::numItems() const
{
    return m_model.numItems();
}

std::size_t NewsList::curItem() const 
{
    const auto& indices = m_ui.tableGroups->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return 0;
    const auto& first = indices.first();
    return first.row();    
}

void NewsList::setFound(std::size_t index)
{
    auto* model = m_ui.tableGroups->model();
    auto i = model->index(index, 0);
    m_ui.tableGroups->setCurrentIndex(i);
    m_ui.tableGroups->scrollTo(i);
}

void NewsList::loadState(app::Settings& settings)
{
    app::loadTableLayout("newslist", m_ui.tableGroups, settings);
    app::loadState("newslist", m_ui.editFilter, settings);
    app::loadState("newslist", m_ui.chkShowEmpty, settings);
    app::loadState("newslist", m_ui.chkShowText, settings);
    app::loadState("newslist", m_ui.chkMusic, settings);
    app::loadState("newslist", m_ui.chkMovies, settings);
    app::loadState("newslist", m_ui.chkTV, settings);
    app::loadState("newslist", m_ui.chkGames, settings);
    app::loadState("newslist", m_ui.chkApps, settings);
    app::loadState("newslist", m_ui.chkAdult, settings);
    app::loadState("newslist", m_ui.chkImages, settings);    
    app::loadState("newslist", m_ui.chkOther, settings);

    accountsUpdated();
}

void NewsList::saveState(app::Settings& settings)
{
    app::saveTableLayout("newslist", m_ui.tableGroups, settings);
    app::saveState("newslist", m_ui.editFilter, settings);
    app::saveState("newslist", m_ui.chkShowEmpty, settings);
    app::saveState("newslist", m_ui.chkShowText, settings);    
    app::saveState("newslist", m_ui.chkMusic, settings);
    app::saveState("newslist", m_ui.chkMovies, settings);
    app::saveState("newslist", m_ui.chkTV, settings);
    app::saveState("newslist", m_ui.chkGames, settings);
    app::saveState("newslist", m_ui.chkApps, settings);
    app::saveState("newslist", m_ui.chkAdult, settings);
    app::saveState("newslist", m_ui.chkImages, settings);
    app::saveState("newslist", m_ui.chkOther, settings);

}

void NewsList::on_actionBrowse_triggered()
{
    const auto* account = app::g_accounts->findAccount(m_curAccount);
    Q_ASSERT(account);

    const auto& indices = m_ui.tableGroups->selectionModel()->selectedRows();
    for (const auto& i : indices)
    {
        const auto& datapath = account->datapath;
        const auto& group    = m_model.getName(i);
        const auto account   = m_curAccount;
        const auto& guid     = QString("%1/%2").arg(account).arg(group);

        bool isTabOpen = false;

        for (std::size_t i=0; i<g_win->numWidgets(); ++i)
        {
            const auto* w = g_win->getWidget(i);
            const auto& p = w->property("newsgroup-guid");
            if (p.isNull()) continue;
            if (p.toString() == guid)
            {
                g_win->focusWidget(w);
                isTabOpen = true;
                break;
            }
        }
        if (isTabOpen) continue;

        auto* news = new NewsGroup(account, datapath, group);
        g_win->attach(news, false, true);
        news->setProperty("newsgroup-guid", guid);        
        news->setProperty("parent-object", QVariant::fromValue(static_cast<QObject*>(this)));
        news->load();        
    }
}


void NewsList::on_actionRefresh_triggered()
{
    m_model.clear();

    const auto* account = app::g_accounts->findAccount(m_curAccount);
    Q_ASSERT(account);

    DEBUG("Refresh newslist '%1' (%2)", account->name, account->id);

    const auto file = app::homedir::file(account->name + ".lst");

    m_ui.progressBar->setMaximum(0);        
    m_ui.progressBar->setVisible(true);
    m_ui.actionStop->setEnabled(true);
    m_ui.actionRefresh->setEnabled(false);
    m_model.makeListing(file, account->id);        

}

void NewsList::on_actionFavorite_triggered()
{
    auto indices = m_ui.tableGroups->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    Q_ASSERT(m_curAccount);

    m_model.subscribe(indices, m_curAccount);
}

void NewsList::on_actionUnfavorite_triggered()
{
    auto indices = m_ui.tableGroups->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    Q_ASSERT(m_curAccount);

    m_model.unsubscribe(indices, m_curAccount);
}

void NewsList::on_actionDeleteData_triggered()
{
    const auto& indices = m_ui.tableGroups->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    QMessageBox msg(this);
    msg.setIcon(QMessageBox::Question);
    msg.setWindowTitle(tr("Delete Group Data"));
    msg.setText(tr("This will delete all the group data. Are you sure?"));
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (msg.exec() == QMessageBox::No)
        return;

    const auto* account = app::g_accounts->findAccount(m_curAccount);
    Q_ASSERT(account);

    for (const auto& index : indices)
    {
        const auto& datapath = account->datapath;
        const auto& group    = m_model.getName(index);
        const auto account   = m_curAccount;
        const auto& guid     = QString("%1/%2").arg(account).arg(group);

        // check if the tab is open
        for (std::size_t i=0; i<g_win->numWidgets(); ++i)
        {
            auto* w = g_win->getWidget(i);
            const auto& p = w->property("newsgroup-guid");
            if (p.isNull()) 
                continue;
            if (p.toString() != guid) 
                continue;

            auto* tab = qobject_cast<NewsGroup*>(w);
            g_win->closeWidget(tab);
            break;
        }
        NewsGroup::deleteData(account, datapath, group);

        m_model.clearSize(index);
    }
}

void NewsList::on_actionStop_triggered()
{
    Q_ASSERT(m_curAccount);

    m_model.stop(m_curAccount);

    m_ui.actionStop->setEnabled(false);    
    m_ui.actionRefresh->setEnabled(true);
    m_ui.progressBar->setVisible(false);
}


void NewsList::on_cmbAccounts_currentIndexChanged()
{
    m_ui.actionStop->setEnabled(false);
    m_ui.actionRefresh->setEnabled(false);

    const auto index = m_ui.cmbAccounts->currentIndex();
    if (index == -1)
        return;

    const auto name = m_ui.cmbAccounts->currentText();
    const auto file = app::homedir::file(name + ".lst");

    const auto numAccounts = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<numAccounts; ++i)
    {
        const auto& acc = app::g_accounts->getAccount(i);
        if (acc.name != name)
            continue;
        if (m_curAccount == acc.id)
            return;

        m_curAccount = acc.id;

        DEBUG("Current account changed to '%1' (%2)", acc.name, acc.id);

        m_ui.progressBar->setMaximum(0);
        m_ui.progressBar->setVisible(true);

        QFileInfo info(file);
        if (info.exists())
        {
            m_model.clear();
            m_model.loadListing(file, acc.id);
            m_ui.actionStop->setEnabled(false);
            m_ui.actionRefresh->setEnabled(true);
        }
        else
        {
            m_model.clear();
            m_model.makeListing(file, acc.id);
            m_ui.actionStop->setEnabled(true);
            m_ui.actionRefresh->setEnabled(false);
        }
        return;
    }
    Q_ASSERT(!"account was not found");
}

void NewsList::on_tableGroups_customContextMenuRequested(QPoint point)
{
    QMenu menu(this);
    menu.addAction(m_ui.actionBrowse);
    menu.addSeparator();
    menu.addAction(m_ui.actionFavorite);
    menu.addAction(m_ui.actionUnfavorite);
    menu.addSeparator();
    menu.addAction(m_ui.actionDeleteData);
    menu.exec(QCursor::pos());
}

void NewsList::on_tableGroups_doubleClicked(const QModelIndex& index)
{
    on_actionBrowse_triggered();    
}

void NewsList::on_editFilter_returnPressed()
{
    filter();
}

void NewsList::on_editFilter_textChanged()
{
    filter();
}

void NewsList::on_chkShowEmpty_clicked()
{
    filter();
}

void NewsList::on_chkShowText_clicked()
{
    filter();
}

void NewsList::on_chkMusic_clicked()
{
    filter();
}
void NewsList::on_chkMovies_clicked()
{
    filter();
}

void NewsList::on_chkTV_clicked()
{
    filter();
}

void NewsList::on_chkGames_clicked()
{
    filter();
}

void NewsList::on_chkApps_clicked()
{
    filter();
}

void NewsList::on_chkImages_clicked()
{
    filter();
}

void NewsList::on_chkOther_clicked()
{
    filter();
}
void NewsList::on_chkAdult_clicked()
{
    filter();
}

void NewsList::accountsUpdated()
{
    m_ui.cmbAccounts->clear();
    m_ui.cmbAccounts->blockSignals(true);

    int currentAccountIndex = -1;

    const auto numAccounts = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<numAccounts; ++i)
    {
        const auto& account = app::g_accounts->getAccount(i);
        if (account.id == m_curAccount)
            currentAccountIndex = (int)i;

        m_ui.cmbAccounts->addItem(account.name);
    }

    if (currentAccountIndex == -1)
    {
        m_model.clear();
        m_curAccount = 0;
    }
    else
    {
        m_ui.cmbAccounts->setCurrentIndex(currentAccountIndex);
    }

    m_ui.cmbAccounts->blockSignals(false);    

    m_ui.actionRefresh->setEnabled(numAccounts != 0);
    m_ui.actionFavorite->setEnabled(numAccounts != 0);
}

void NewsList::progressUpdated(quint32 acc, quint32 maxValue, quint32 curValue)
{
    if (m_curAccount != acc)
        return;

    m_ui.progressBar->setMaximum(maxValue);
    m_ui.progressBar->setValue(curValue);
}

void NewsList::loadComplete(quint32 acc)
{
    DEBUG("Load complete %1", acc);

    if (m_curAccount != acc)
        return;

    m_ui.progressBar->setVisible(false);

    resort();
    filter();
}

void NewsList::makeComplete(quint32 accountId)
{
    DEBUG("Make complete %1", accountId);

    if (m_curAccount != accountId)
        return;

    m_ui.progressBar->setVisible(false);
    m_ui.actionStop->setEnabled(false);

    const auto numAcccounts = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<numAcccounts; ++i)
    {
        const auto& acc = app::g_accounts->getAccount(i);
        if (acc.id != accountId)
            continue;

        const auto& name = acc.name;
        const auto& file = app::homedir::file(name + ".lst");
        m_model.loadListing(file, acc.id);
        return;
    }

    Q_ASSERT(!"Account was not found");
}

void NewsList::resort()
{
    const QHeaderView* header = m_ui.tableGroups->horizontalHeader();
    const auto sortColumn = header->sortIndicatorSection();
    const auto sortOrder  = header->sortIndicatorOrder();
    m_ui.tableGroups->sortByColumn(sortColumn, sortOrder);
}

void NewsList::filter()
{
    const auto& str = m_ui.editFilter->text();
    const auto showMusic      = m_ui.chkMusic->isChecked();
    const auto showMovies     = m_ui.chkMovies->isChecked();
    const auto showTelevision = m_ui.chkTV->isChecked();
    const auto showGames      = m_ui.chkGames->isChecked();
    const auto showApps       = m_ui.chkApps->isChecked();
    const auto showAdult      = m_ui.chkAdult->isChecked();
    const auto showImages     = m_ui.chkImages->isChecked();
    const auto showOther      = m_ui.chkOther->isChecked();
    const auto showEmpty      = m_ui.chkShowEmpty->isChecked();
    const auto showText       = m_ui.chkShowText->isChecked();

    using F = app::NewsList::FilterFlags;

    newsflash::bitflag<F> flags;
    flags.set(F::ShowMusic, showMusic);
    flags.set(F::ShowMovies, showMovies);
    flags.set(F::ShowTv, showTelevision);
    flags.set(F::ShowGames, showGames);
    flags.set(F::ShowApps, showApps);
    flags.set(F::ShowAdult, showAdult);
    flags.set(F::ShowImages, showImages),
    flags.set(F::ShowOther, showOther);
    flags.set(F::ShowEmpty, showEmpty);
    flags.set(F::ShowText, showText);

    m_model.filter(str, flags);
}

} // gui