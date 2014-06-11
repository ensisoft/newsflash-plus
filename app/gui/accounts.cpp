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
#include <newsflash/sdk/format.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/datastore.h>
#include <newsflash/sdk/message.h>
#include <newsflash/sdk/message_account.h>
#include <ctime>

#include "dlgaccount.h"
#include "accounts.h"
#include "message.h"

#include "../accounts.h"


using sdk::str;

namespace {
    app::accounts& get_accounts_model(sdk::model& m)
    {
        return static_cast<app::accounts&>(m);
    }
}// 

namespace gui
{

void openurl(const QString&);

Accounts::Accounts(sdk::model& model) : model_(model)
{
    ui_.setupUi(this);
    ui_.listView->setModel(model.view());
    ui_.lblMovie->installEventFilter(this);

    const bool empty = model.is_empty();

    ui_.actionDel->setEnabled(!empty);
    ui_.actionProperties->setEnabled(!empty);

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

    sdk::listen<msg_first_launch>(this);
    sdk::listen<sdk::msg_account_downloads_update>(this);
    sdk::listen<sdk::msg_account_quota_update>(this);
}

Accounts::~Accounts()
{
    ui_.lblMovie->removeEventFilter(this);
}

void Accounts::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionNew);
    menu.addAction(ui_.actionDel);
    menu.addSeparator();
    menu.addAction(ui_.actionProperties);
}

void Accounts::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionNew);
    bar.addAction(ui_.actionDel);
    bar.addSeparator();
    bar.addAction(ui_.actionProperties);
}

sdk::widget::info Accounts::information() const
{
    return {"accounts.html", true};
}

void Accounts::load(const sdk::datastore& data)
{
    const auto license  = data.get("accounts", "license", "");
    const bool validate = keygen::verify_code(license);
    if (validate)
        advertise(false);
    else advertise(true);

    license_ = license;
}

void Accounts::save(sdk::datastore& data)
{
    data.set("accounts", "license", license_);
}

void Accounts::advertise(bool show)
{
    ui_.lblMovie->setMovie(nullptr);
    ui_.lblMovie->setVisible(false);
    ui_.lblPlead->setVisible(false);
    ui_.lblRegister->setVisible(false);
    movie_.release();

    if (!show) return;

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

    // NOTE: if the movie doesn't show up the problem might have
    // to do with Qt image plugins!    
    movie_.reset(new QMovie(resource));
    Q_ASSERT(movie_->isValid());

    DEBUG(str("Usenet campaing '_1' '_2'", resource, campaing));

    movie_->start();
    movie_->setSpeed(200);    

    const auto& pix  = movie_->currentPixmap();
    const auto& size = pix.size();
    ui_.lblMovie->setMinimumSize(size);
    ui_.lblMovie->resize(size);
    ui_.lblMovie->setMovie(movie_.get());
    ui_.lblMovie->setVisible(true);
    ui_.lblMovie->setProperty("url", campaing);    
    ui_.lblPlead->setVisible(true);
    ui_.lblRegister->setVisible(true);
}

void Accounts::on_message(const char*, msg_first_launch& msg)
{
    if (!msg.add_account)
        return;

    auto& accounts = get_accounts_model(model_);
    auto account   = accounts.suggest();

    DlgAccount dlg(this, account);
    if (dlg.exec() == QDialog::Accepted)
        accounts.set(account);

}

void Accounts::on_message(const char*, sdk::msg_account_downloads_update& msg)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    const auto& accounts = get_accounts_model(model_);
    const auto& account  = accounts.get(row);
    if (account.id() != msg.id)
        return;

    // update gui 
    currentRowChanged();
}

void Accounts::on_message(const char*, sdk::msg_account_quota_update& msg)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    const auto& accounts = get_accounts_model(model_);
    const auto& account  = accounts.get(row);
    if (account.id() != msg.id)
        return;

    // update gui
    currentRowChanged();
}

bool Accounts::eventFilter(QObject* object, QEvent* event)
{
    if (object == ui_.lblMovie &&
        event->type() == QEvent::MouseButtonPress)
    {
        const auto& url = ui_.lblMovie->property("url").toString();
        openurl(url);
        return true;
    }
    return QObject::eventFilter(object, event);
}

void Accounts::on_actionNew_triggered()
{
    auto& accounts = get_accounts_model(model_);
    auto account   = accounts.suggest();

    DlgAccount dlg(this, account);
    if (dlg.exec() == QDialog::Accepted)
        accounts.set(account);
}

void Accounts::on_actionDel_triggered()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& accounts = get_accounts_model(model_);
    accounts.del(row);
}

void Accounts::on_actionProperties_triggered()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& accounts = get_accounts_model(model_);
    auto account   = accounts.get(row);

    DlgAccount dlg(this, account);
    if (dlg.exec() == QDialog::Accepted)
        accounts.set(account);
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
        ui_.actionProperties->setEnabled(false);
        return;
    }

    ui_.grpServer->setEnabled(true);
    ui_.grpQuota->setEnabled(true);
    ui_.actionDel->setEnabled(true);
    ui_.actionProperties->setEnabled(true);

    auto& accounts  = get_accounts_model(model_);
    const auto& acc = accounts.get(row);

    const auto quota_type   = acc.quota_type();            
    const auto quota_total  = sdk::gigs(acc.quota_total());
    const auto quota_spent  = sdk::gigs(acc.quota_spent());
    const auto quota_avail  = sdk::gigs(acc.quota_avail());
    const auto gigs_alltime = sdk::gigs(acc.downloads_all_time());
    const auto gigs_month   = sdk::gigs(acc.downloads_this_month());

    ui_.edtMonth->setText(str(gigs_month));
    ui_.edtAllTime->setText(str(gigs_alltime));
    ui_.spinTotal->setValue(quota_total.as_float());
    ui_.spinSpent->setValue(quota_spent.as_float());
    
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

    QStandardItemModel* model = static_cast<QStandardItemModel*>(ui_.pie->model());

    if (model->rowCount())
    {
        Q_ASSERT(model->rowCount() == 2);
        model->removeRow(0);
        model->removeRow(0);
    }

    const auto slice_avail = 100 * (quota_avail.as_float() / quota_total.as_float());
    const auto slice_used  = 100 * (quota_spent.as_float() / quota_total.as_float());

    model->insertRows(0, 1, QModelIndex());
    model->insertRows(1, 1, QModelIndex());
    model->setData(model->index(0, 0), "Avail");
    model->setData(model->index(0, 1), slice_avail);
    model->setData(model->index(0, 0), 
        QColor(0, 0x80, 0), Qt::DecorationRole);

    model->setData(model->index(1, 0), "Used");
    model->setData(model->index(1, 1), slice_used);    
    model->setData(model->index(1, 0),
        QColor(0x80, 0, 0), Qt::DecorationRole);
}

void Accounts::on_btnResetMonth_clicked()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& accounts = get_accounts_model(model_);
    auto& account  = accounts.get(row);

    account.reset_month();

    const sdk::gigs nada;

    ui_.edtMonth->setText(str(nada));
}

void Accounts::on_btnResetAllTime_clicked()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& accounts = get_accounts_model(model_);
    auto& account  = accounts.get(row);

    account.reset_all_time();

    const sdk::gigs nada;

    ui_.edtAllTime->setText(str(nada));
}


void Accounts::on_spinTotal_valueChanged(double value)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& accounts = get_accounts_model(model_);
    auto& account  = accounts.get(row);

    account.quota_total(value);

}

void Accounts::on_spinSpent_valueChanged(double value)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& accounts = get_accounts_model(model_);
    auto& account  = accounts.get(row);    

    account.quota_spent(value);
}

void Accounts::on_listView_doubleClicked(const QModelIndex& index)
{
    // forward
    on_actionProperties_triggered();
}

void Accounts::on_listView_customContextMenuRequested(QPoint pos)
{
    QMenu menu(this);
    add_actions(menu);
    menu.exec(QCursor::pos());
}

void Accounts::on_grpQuota_toggled(bool on)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& accounts = get_accounts_model(model_);
    auto& account  = accounts.get(row);

    if (!on)
    {
        account.quota_type(app::account::quota::none);
    }
    else 
    {
        if (ui_.btnFixedQuota->isChecked())
            account.quota_type(app::account::quota::fixed);
        else account.quota_type(app::account::quota::monthly);
    }
}

} // gui
