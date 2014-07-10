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
#include "message.h"

#include "../accounts.h"
#include "../datastore.h"
#include "../debug.h"
#include "../format.h"
#include "../message.h"

using app::str;

namespace gui
{

void openurl(const QString&);

accounts::accounts(app::accounts& model) : model_(model)
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

    app::listen<msg_first_launch>(this);
    app::listen<app::msg_account_downloads_update>(this);
    app::listen<app::msg_account_quota_update>(this);
}

accounts::~accounts()
{
    ui_.lblMovie->removeEventFilter(this);
}

void accounts::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionNew);
    menu.addAction(ui_.actionDel);
    menu.addSeparator();
    menu.addAction(ui_.actionProperties);
}

void accounts::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionNew);
    bar.addAction(ui_.actionDel);
    bar.addSeparator();
    bar.addAction(ui_.actionProperties);
}

mainwidget::info accounts::information() const
{
    return {"accounts.html", true};
}

void accounts::load(const app::datastore& data)
{
    const auto license  = data.get("accounts", "license", "");
    const bool validate = keygen::verify_code(license);
    if (validate)
        advertise(false);
    else advertise(true);

    license_ = license;
}

void accounts::save(app::datastore& data)
{
    data.set("accounts", "license", license_);
}

void accounts::advertise(bool show)
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

void accounts::on_message(const char*, msg_first_launch& msg)
{
    if (!msg.add_account)
        return;

    auto account = model_.suggest();

    DlgAccount dlg(this, account);
    if (dlg.exec() == QDialog::Accepted)
        model_.set(account);

}

void accounts::on_message(const char*, app::msg_account_downloads_update& msg)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    const auto& account  = model_.get(row);
    if (account.id() != msg.id)
        return;

    // update gui 
    currentRowChanged();
}

void accounts::on_message(const char*, app::msg_account_quota_update& msg)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    const auto& account = model_.get(row);
    if (account.id() != msg.id)
        return;

    // update gui
    currentRowChanged();
}

bool accounts::eventFilter(QObject* object, QEvent* event)
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

void accounts::on_actionNew_triggered()
{
    auto account  = model_.suggest();

    DlgAccount dlg(this, account);
    if (dlg.exec() == QDialog::Accepted)
        model_.set(account);
}

void accounts::on_actionDel_triggered()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    model_.del(row);
}

void accounts::on_actionProperties_triggered()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto account = model_.get(row);

    DlgAccount dlg(this, account);
    if (dlg.exec() == QDialog::Accepted)
        model_.set(account);
}

void accounts::currentRowChanged()
{
    QStandardItemModel* model = static_cast<QStandardItemModel*>(ui_.pie->model());
    if (model->rowCount())
    {
        Q_ASSERT(model->rowCount() == 2);
        model->removeRow(0);
        model->removeRow(0);
    }    

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

    in_row_changed_ = true;

    ui_.grpServer->setEnabled(true);
    ui_.grpQuota->setEnabled(true);
    ui_.actionDel->setEnabled(true);
    ui_.actionProperties->setEnabled(true);

    const auto& account = model_.get(row);

    const auto quota_type   = account.quota_type();            
    const auto quota_total  = app::gigs(account.quota_total());
    const auto quota_spent  = app::gigs(account.quota_spent());
    const auto quota_avail  = app::gigs(account.quota_avail());
    const auto gigs_alltime = app::gigs(account.downloads_all_time());
    const auto gigs_month   = app::gigs(account.downloads_this_month());

    ui_.edtMonth->setText(str(gigs_month));
    ui_.edtAllTime->setText(str(gigs_alltime));
    ui_.spinTotal->setValue(quota_total.as_float());
    ui_.spinSpent->setValue(quota_spent.as_float());
    ui_.spinSpent->setMaximum(quota_total.as_float());
    
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

    const auto slice_avail = 100 * (quota_avail.as_float() / quota_total.as_float());
    const auto slice_used  = 100 * (quota_spent.as_float() / quota_total.as_float());

    DEBUG(str("quota avail _1, used _2", slice_avail, slice_used));

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

    in_row_changed_ = false;
}

void accounts::on_btnResetMonth_clicked()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = model_.get(row);

    account.reset_month();

    const app::gigs nada;

    ui_.edtMonth->setText(str(nada));
}

void accounts::on_btnResetAllTime_clicked()
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = model_.get(row);

    account.reset_all_time();

    const app::gigs nada;

    ui_.edtAllTime->setText(str(nada));
}

void accounts::on_btnMonthlyQuota_toggled(bool checked)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = model_.get(row);

    account.quota_type(app::account::quota::monthly);

    ui_.btnFixedQuota->setChecked(false);
}

void accounts::on_btnFixedQuota_toggled(bool checked)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = model_.get(row);

    account.quota_type(app::account::quota::fixed);

    ui_.btnMonthlyQuota->setChecked(false);
}

void accounts::on_spinTotal_valueChanged(double value)
{
    if (in_row_changed_)
        return;

    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = model_.get(row);

    DEBUG(str("Quota total value _1", value));

    const app::gigs gigs { value };

    account.quota_total(gigs.as_bytes());

    currentRowChanged();
}

void accounts::on_spinSpent_valueChanged(double value)
{
    if (in_row_changed_)
        return;

    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    DEBUG(str("Quota spent value _1", value));

    auto& account = model_.get(row);    

    const app::gigs gigs { value };

    account.quota_spent(gigs.as_bytes());

    currentRowChanged();
}

void accounts::on_listView_doubleClicked(const QModelIndex& index)
{
    // forward
    on_actionProperties_triggered();
}

void accounts::on_listView_customContextMenuRequested(QPoint pos)
{
    QMenu menu(this);
    add_actions(menu);
    menu.exec(QCursor::pos());
}

void accounts::on_grpQuota_toggled(bool on)
{
    const auto row = ui_.listView->currentIndex().row();
    if (row == -1)
        return;

    auto& account = model_.get(row);

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
