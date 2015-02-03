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
#  include <QtGui/QMessageBox>
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#  include <QtGui/QPixmap>
#  include <QtGui/QMovie>
#  include <QFile>
#  include <QFileInfo>
#include <newsflash/warnpop.h>

#include "groups.h"
#include "../accounts.h"
#include "../homedir.h"
#include "../settings.h"

namespace gui
{

Groups::Groups()
{
    ui_.setupUi(this);
    ui_.tableGroups->setModel(&model_);
    ui_.tableGroups->setColumnWidth(0, 150);
    ui_.progressBar->setVisible(false);

    ui_.actionRefresh->setShortcut(QKeySequence::Refresh);

    QObject::connect(app::g_accounts, SIGNAL(accountsUpdated()),
        this, SLOT(accountsUpdated()));

    accountsUpdated();
}

Groups::~Groups()
{}

void Groups::addActions(QMenu& menu)
{

}

void Groups::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionRefresh);
}

MainWidget::info Groups::getInformation() const
{
    return {"groups.html", true, true};
}

void Groups::loadState(app::Settings& settings)
{
    const auto subscribedOnly = settings.get("groups", "show_subscribed_only", false);
    ui_.chkSubscribedOnly->setChecked(subscribedOnly);

    accountsUpdated();
}

bool Groups::saveState(app::Settings& settings)
{
    const auto subscribedOnly = ui_.chkSubscribedOnly->isChecked();

    settings.set("groups", "show_subscribed_only", subscribedOnly);

    return true;
}

void Groups::on_actionRefresh_triggered()
{
    model_.clear();

    int index = ui_.cmbAccounts->currentIndex();
    if (index == -1)
        return;

    const auto name = ui_.cmbAccounts->currentText();
    const auto file = app::homedir::file(name + ".lst");

    const auto numAccounts = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<numAccounts; ++i)
    {
        const auto& acc = app::g_accounts->getAccount(i);
        if (acc.name == name)
        {
            model_.makeListing(file, acc.id);
            return;
        }
    }
    Q_ASSERT(!"account was not found");
}


void Groups::on_cmbAccounts_currentIndexChanged()
{
    return; 

    model_.clear();

    int index = ui_.cmbAccounts->currentIndex();
    if (index == -1)
        return;

    //const QVariant user = ui_.cmbAccounts->userData(index);

    const QString text = ui_.cmbAccounts->currentText();
    const QString file = app::homedir::file(text + ".lst");

    QFileInfo info(file);
    if (!info.exists())
    {
        model_.loadListing(file, 0);
    }
    else
    {
        model_.makeListing(file, 0);
        ui_.progressBar->setVisible(true);
        ui_.progressBar->setMinimum(0),
        ui_.progressBar->setMaximum(0);
        ui_.progressBar->setValue(0);
    }
}

void Groups::accountsUpdated()
{
    ui_.cmbAccounts->clear();

    const auto numAccounts = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<numAccounts; ++i)
    {
        const auto& account = app::g_accounts->getAccount(i);
        ui_.cmbAccounts->addItem(account.name, account.id);
    }
}

} // gui