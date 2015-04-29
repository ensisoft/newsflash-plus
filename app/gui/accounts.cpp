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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#  include <QtGui/QPixmap>
#  include <QtGui/QMovie>
#  include <QtGui/QStandardItemModel>
#include <newsflash/warnpop.h>
#include <newsflash/keygen/keygen.h>
#include <ctime>
#include "dlgaccount.h"
#include "accounts.h"
#include "../accounts.h"
#include "../settings.h"
#include "../debug.h"
#include "../format.h"
#include "../platform.h"
#include "../types.h"

namespace gui
{

Accounts::Accounts()
{
    const bool empty = app::g_accounts->numAccounts() == 0;

    ui_.setupUi(this);
    ui_.listView->setModel(app::g_accounts);
    ui_.lblMovie->installEventFilter(this);
    ui_.actionDel->setEnabled(!empty);
    ui_.actionEdit->setEnabled(!empty);
    ui_.grpServer->setEnabled(false);
    ui_.grpQuota->setEnabled(false);
    ui_.grpQuota->setChecked(false);    
    ui_.btnMonthlyQuota->setChecked(true);
    ui_.pie->setScale(0.8);
    ui_.pie->setDrawPie(false);

    auto* selection = ui_.listView->selectionModel();
    QObject::connect(selection, SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(currentRowChanged()));

    QObject::connect(app::g_accounts, SIGNAL(accountsUpdated()),
        this, SLOT(updatePie()));
    QObject::connect(app::g_accounts, SIGNAL(accountsUpdated()),
        this, SLOT(currentRowChanged()));

    qsrand(std::time(nullptr));

    QString resource;
    QString campaing;    
    if ((qrand() >> 7) & 1)
    {
        resource = ":/resource/uns-special-2.gif";
        campaing = "https://usenetserver.com/partners/?a_aid=foobar1234&amp;a_bid=dcec941d";
    }
    else
    {
        resource = ":/resource/nh-special.gif";
        campaing = "http://www.newshosting.com/en/index.php?&amp;a_aid=foobar1234&amp;a_bid=2b57ce3a";
    }    
    //DEBUG("Usenet campaing '%1' '%2'", resource, campaing);    

    // NOTE: if the movie doesn't show up the problem might have
    // to do with Qt image plugins!    

    QMovie* mov = new QMovie(this);
    mov->setFileName(resource);
    mov->start();
    mov->setSpeed(200);    

    const auto& pix  = mov->currentPixmap();
    const auto& size = pix.size();
    ui_.lblMovie->setMinimumSize(size);
    ui_.lblMovie->resize(size);
    ui_.lblMovie->setMovie(mov);
    ui_.lblMovie->setVisible(true);
    ui_.lblMovie->setProperty("url", campaing);    
    ui_.lblPlead->setVisible(true);
    ui_.lblRegister->setVisible(true);
}

Accounts::~Accounts()
{
    ui_.lblMovie->removeEventFilter(this);
}

void Accounts::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionAdd);
    menu.addAction(ui_.actionDel);
    menu.addSeparator();
    menu.addAction(ui_.actionEdit);
}

void Accounts::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionAdd);
    bar.addAction(ui_.actionDel);
    bar.addSeparator();
    bar.addAction(ui_.actionEdit);
}

MainWidget::info Accounts::getInformation() const
{
    return {"accounts.html", true};
}

void Accounts::loadState(app::Settings& s)
{
    const auto license  = s.get("accounts", "license", "");
    const bool validate = keygen::verify_code(license);
    if (validate)
    {
        ui_.lblMovie->setMovie(nullptr);
        ui_.lblMovie->setVisible(false);
        ui_.lblPlead->setVisible(false);
        ui_.lblRegister->setVisible(false);
    }
}

void Accounts::saveState(app::Settings& s)
{
}


bool Accounts::eventFilter(QObject* object, QEvent* event)
{
    if (object == ui_.lblMovie &&
        event->type() == QEvent::MouseButtonPress)
    {
        const auto& url = ui_.lblMovie->property("url").toString();
        app::openWeb(url);
        return true;
    }
    return QObject::eventFilter(object, event);
}

void Accounts::updatePie()
{
    ui_.pie->setDrawPie(false);
    ui_.pie->setScale(0.8);

    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    const auto& account = app::g_accounts->getAccount(row);
    if (account.quotaType != app::Accounts::Quota::none)
    {
        const auto quota_spent = app::gigs(account.quotaSpent);
        const auto quota_total = app::gigs(account.quotaAvail);
        auto slice_used  = quota_spent.as_float() / quota_total.as_float();
        if (slice_used > 1.0)
            slice_used = 1.0;
        auto slice_avail = 1.0 - slice_used;
        ui_.pie->setAvailSlice(slice_avail);        
        ui_.pie->setDrawPie(true);

        DEBUG("Quota avail %1, used %2", slice_avail, slice_used);
    }
}


void Accounts::on_actionAdd_triggered()
{
    auto account  = app::g_accounts->suggestAccount();

    DlgAccount dlg(this, account);
    if (dlg.exec() == QDialog::Accepted)
        app::g_accounts->setAccount(account);
}

void Accounts::on_actionDel_triggered()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    app::g_accounts->delAccount(row);
}

void Accounts::on_actionEdit_triggered()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto account = app::g_accounts->getAccount(row);

    DlgAccount dlg(this, account);
    if (dlg.exec() == QDialog::Accepted)
        app::g_accounts->setAccount(account);
}

void Accounts::currentRowChanged()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
    {
        ui_.spinThisMonth->setValue(0);
        ui_.spinAllTime->setValue(0);
        ui_.grpServer->setEnabled(false);
        ui_.grpQuota->setEnabled(false);
        ui_.spinQuotaUsed->setValue(0);
        ui_.spinQuotaAvail->setValue(0);
        ui_.actionDel->setEnabled(false);
        ui_.actionEdit->setEnabled(false);
        return;
    }

    ui_.grpServer->setEnabled(true);
    ui_.grpQuota->setEnabled(true);
    ui_.actionDel->setEnabled(true);
    ui_.actionEdit->setEnabled(true);

    const auto& account = app::g_accounts->getAccount(row);

    const auto quota_type   = account.quotaType;
    const auto quota_spent  = app::gigs(account.quotaSpent);
    const auto quota_total  = app::gigs(account.quotaAvail);
    const auto gigs_alltime = app::gigs(account.downloadsAllTime);
    const auto gigs_month   = app::gigs(account.downloadsThisMonth);

    in_row_changed_ = true;

    ui_.spinAllTime->setValue(gigs_alltime.as_float());
    ui_.spinThisMonth->setValue(gigs_month.as_float());
    ui_.spinQuotaAvail->setValue(quota_total.as_float());
    ui_.spinQuotaUsed->setMaximum(quota_total.as_float());        
    ui_.spinQuotaUsed->setValue(quota_spent.as_float());

    in_row_changed_ = false;

    
    if (quota_type == app::Accounts::Quota::none)
    {
        ui_.grpQuota->setChecked(false);
    }
    else if (quota_type == app::Accounts::Quota::monthly)
    {
        ui_.btnMonthlyQuota->setChecked(true);
        ui_.btnFixedQuota->setChecked(false);
        ui_.grpQuota->setChecked(true);
    }
    else
    {
        ui_.btnMonthlyQuota->setChecked(false);
        ui_.btnFixedQuota->setChecked(true);
        ui_.grpQuota->setChecked(true);        
    }

    updatePie();
}

void Accounts::on_btnResetMonth_clicked()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    account.downloadsThisMonth = 0;

    ui_.spinThisMonth->setValue(0);
}

void Accounts::on_btnResetAllTime_clicked()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    account.downloadsAllTime = 0;

    ui_.spinAllTime->setValue(0);
}

void Accounts::on_btnMonthlyQuota_toggled(bool checked)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    account.quotaType = app::Accounts::Quota::monthly;

    ui_.btnFixedQuota->setChecked(false);
}

void Accounts::on_btnFixedQuota_toggled(bool checked)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    account.quotaType = app::Accounts::Quota::fixed;

    ui_.btnMonthlyQuota->setChecked(false);
}

void Accounts::on_spinThisMonth_valueChanged(double value)
{
    if (in_row_changed_)
        return;
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    const app::gigs gigs { value };

    account.downloadsThisMonth = gigs.as_bytes();
    account.lastUseDate = QDate::currentDate();
}

void Accounts::on_spinAllTime_valueChanged(double value)
{
    if (in_row_changed_)
        return;
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    const app::gigs gigs { value };

    account.downloadsAllTime = gigs.as_bytes();
}

void Accounts::on_spinQuotaAvail_valueChanged(double value)
{
    if (in_row_changed_)
        return;

    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    DEBUG("Quota available %1", value);

    const app::gigs gigs { value };

    account.quotaAvail = gigs.as_bytes();

    ui_.spinQuotaUsed->setMaximum(gigs.as_float());

    updatePie();
}

void Accounts::on_spinQuotaUsed_valueChanged(double value)
{
    if (in_row_changed_)
        return;

    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    DEBUG("Quota spent value %1", value);

    auto& account = app::g_accounts->getAccount(row);    

    const app::gigs gigs { value };

    account.quotaSpent = gigs.as_bytes();

    updatePie();
}

void Accounts::on_listView_doubleClicked(const QModelIndex& index)
{
    // forward
    on_actionEdit_triggered();
}

void Accounts::on_listView_customContextMenuRequested(QPoint pos)
{
    QMenu menu(this);
    addActions(menu);
    menu.exec(QCursor::pos());
}

void Accounts::on_grpQuota_toggled(bool on)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    if (!on)
    {
        account.quotaType = app::Accounts::Quota::none;
    }
    else 
    {
        if (ui_.btnFixedQuota->isChecked())
            account.quotaType = app::Accounts::Quota::fixed;
        else account.quotaType = app::Accounts::Quota::monthly;
    }

    ui_.pie->setDrawPie(on);
}

} // gui
