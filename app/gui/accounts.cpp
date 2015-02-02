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

using app::str;

namespace gui
{

Accounts::Accounts()
{
    ui_.setupUi(this);
    ui_.listView->setModel(app::g_accounts);
    ui_.lblMovie->installEventFilter(this);

    const bool empty = app::g_accounts->numAccounts() == 0;

    ui_.actionDel->setEnabled(!empty);
    ui_.actionEdit->setEnabled(!empty);

    ui_.grpServer->setEnabled(false);
    ui_.grpQuota->setEnabled(false);
    ui_.grpQuota->setChecked(false);    

    ui_.btnMonthlyQuota->setChecked(true);

    auto* selection = ui_.listView->selectionModel();
    QObject::connect(selection, SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(currentRowChanged()));

    QStandardItemModel* pie = new QStandardItemModel(2, 2, this);
    pie->setHeaderData(0, Qt::Horizontal, tr("Available"));
    pie->setHeaderData(1, Qt::Horizontal, tr("Used"));
    ui_.pie->setModel(pie);

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
    DEBUG(str("Usenet campaing '_1' '_2'", resource, campaing));    

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
    return {"accounts.html", true, true};
}

void Accounts::loadState(app::settings& s)
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

bool Accounts::saveState(app::settings& s)
{
    return true;
}

void Accounts::firstLaunch(bool add_account)
{
    if (!add_account)
        return;

    auto account = app::g_accounts->suggestAccount();

    DlgAccount dlg(this, account);
    if (dlg.exec() == QDialog::Accepted)
        app::g_accounts->setAccount(account);
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
    QStandardItemModel* pie = static_cast<QStandardItemModel*>(ui_.pie->model());
    if (pie->rowCount())
    {
        Q_ASSERT(pie->rowCount() == 2);
        pie->removeRow(0);
        pie->removeRow(0);
    }    

    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    const auto& account = app::g_accounts->getAccount(row);    

    const auto quota_spent = app::gigs(account.quota_spent);
    const auto quota_total = app::gigs(account.quota_avail);
    const auto quota_avail = app::gigs(account.quota_avail - account.quota_spent);
    const auto slice_avail = 100 * (quota_avail.as_float() / quota_total.as_float());
    const auto slice_used  = 100 * (quota_spent.as_float() / quota_total.as_float());

    DEBUG(str("quota avail _1, used _2", slice_avail, slice_used));

    pie->insertRows(0, 1, QModelIndex());
    pie->insertRows(1, 1, QModelIndex());
    pie->setData(pie->index(0, 0), "Avail");
    pie->setData(pie->index(0, 1), slice_avail);
    pie->setData(pie->index(0, 0), QColor(0, 0x80, 0), Qt::DecorationRole);
    pie->setData(pie->index(1, 0), "Used");
    pie->setData(pie->index(1, 1), slice_used);    
    pie->setData(pie->index(1, 0), QColor(0x80, 0, 0), Qt::DecorationRole);

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
        ui_.edtMonth->clear();
        ui_.edtAllTime->clear();
        ui_.grpServer->setEnabled(false);
        ui_.grpQuota->setEnabled(false);
        ui_.spinTotal->setValue(0);
        ui_.spinSpent->setValue(0);
        ui_.actionDel->setEnabled(false);
        ui_.actionEdit->setEnabled(false);
        return;
    }

    ui_.grpServer->setEnabled(true);
    ui_.grpQuota->setEnabled(true);
    ui_.actionDel->setEnabled(true);
    ui_.actionEdit->setEnabled(true);

    const auto& account = app::g_accounts->getAccount(row);

    const auto quota_type   = account.quota_type;
    const auto quota_spent  = app::gigs(account.quota_spent);
    const auto quota_total  = app::gigs(account.quota_avail);
    //const auto quota_avail  = app::gigs(account.quota_avail - account.quota_spent);
    const auto gigs_alltime = app::gigs(account.downloads_all_time);
    const auto gigs_month   = app::gigs(account.downloads_this_month);

    in_row_changed_ = true;

    ui_.edtMonth->setText(str(gigs_month));
    ui_.edtAllTime->setText(str(gigs_alltime));
    ui_.spinTotal->setValue(quota_total.as_float());
    ui_.spinSpent->setValue(quota_spent.as_float());
    ui_.spinSpent->setMaximum(quota_total.as_float());    

    in_row_changed_ = false;

    
    if (quota_type == app::account::quota::none)
    {
        ui_.grpQuota->setChecked(false);
    }
    else if (quota_type == app::account::quota::monthly)
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

    account.downloads_this_month = 0;

    const app::gigs nada;

    ui_.edtMonth->setText(str(nada));
}

void Accounts::on_btnResetAllTime_clicked()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    account.downloads_all_time = 0;

    const app::gigs nada;

    ui_.edtAllTime->setText(str(nada));
}

void Accounts::on_btnMonthlyQuota_toggled(bool checked)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    account.quota_type = app::account::quota::monthly;

    ui_.btnFixedQuota->setChecked(false);
}

void Accounts::on_btnFixedQuota_toggled(bool checked)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    account.quota_type = app::account::quota::fixed;

    ui_.btnMonthlyQuota->setChecked(false);
}

void Accounts::on_spinTotal_valueChanged(double value)
{
    if (in_row_changed_)
        return;

    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = app::g_accounts->getAccount(row);

    DEBUG(str("Quota available value _1", value));

    const app::gigs gigs { value };

    account.quota_avail = gigs.as_bytes();

    ui_.spinSpent->setMaximum(gigs.as_float());

    updatePie();
}

void Accounts::on_spinSpent_valueChanged(double value)
{
    if (in_row_changed_)
        return;

    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    DEBUG(str("Quota spent value _1", value));

    auto& account = app::g_accounts->getAccount(row);    

    const app::gigs gigs { value };

    account.quota_spent = gigs.as_bytes();

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
        account.quota_type = app::account::quota::none;
    }
    else 
    {
        if (ui_.btnFixedQuota->isChecked())
            account.quota_type = app::account::quota::fixed;
        else account.quota_type = app::account::quota::monthly;
    }
}

} // gui
