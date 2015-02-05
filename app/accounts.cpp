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

#define LOGTAG "acc"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QFont>
#  include <QtGui/QIcon>
#  include <QDir>
#  include <QStringList>
#include <newsflash/warnpop.h>

#include <ctime>
#include <algorithm>

#include "accounts.h"
#include "format.h"
#include "eventlog.h"
#include "debug.h"
#include "settings.h"
#include "engine.h"

namespace app
{

quint32 CurrentAccountId = 1;

Accounts::Accounts()
{
    mainAccount_ = 0;
    fillAccount_ = 0;
    DEBUG("Accounts created");
}

Accounts::~Accounts()
{
    DEBUG("Accounts deleted");
}

Account Accounts::suggestAccount() const
{
    QString name;

    int index = 1;
    for (;;)
    {
        name = str("My Usenet _1", index);

        const auto& it = std::find_if(std::begin(accounts_), std::end(accounts_),
            [&](const Account& acc) {
                return acc.name == name;
            });
        if (it == std::end(accounts_))
            break;
        ++index;
    }

    Account next = {0};
    next.id                   = CurrentAccountId++;
    next.name                 = name;
    next.general_port         = 119;
    next.secure_port          = 563;
    next.enable_compression   = false;
    next.enable_secure_server = false;
    next.enable_compression   = false;
    next.enable_pipelining    = false;
    next.enable_login         = false;
    next.quota_spent          = 0;
    next.quota_avail          = 0;
    next.downloads_all_time   = 0;
    next.downloads_this_month = 0;
    next.quota_type           = Account::quota::none;
    next.max_connections      = 5;
    return next;
}

const 
Account& Accounts::getAccount(std::size_t index) const
{
    return accounts_[index];
}

Account& Accounts::getAccount(std::size_t index) 
{
    return accounts_[index];
}

const Account* Accounts::getFillAccount() const 
{
    if (fillAccount_ == 0)
        return nullptr;

    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const Account& a) {
            return a.id == fillAccount_;
        });
    Q_ASSERT(it != std::end(accounts_));

    return &(*it);
}

const Account* Accounts::getMainAccount() const
{
    if (accounts_.empty())
        return nullptr;

    if (accounts_.size() == 1)
        return &accounts_[0];

    if (mainAccount_ == 0)
        return nullptr;

    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const Account& a) {
            return a.id == mainAccount_;
        });
    Q_ASSERT(it != std::end(accounts_));

    return &(*it);
}

Account& Accounts::findAccount(quint32 accountId)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const Account& a) {
            return a.id == accountId;
        });
    Q_ASSERT(it != std::end(accounts_));

    return *it;
}

Account& Accounts::findAccount(const QString& name)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const Account& a) { 
            return a.name == name; 
        });
    Q_ASSERT(it != std::end(accounts_));

    return *it;
}

const Account& Accounts::findAccount(quint32 accountId) const
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const Account& a) {
            return a.id == accountId;
        });
    Q_ASSERT(it != std::end(accounts_));

    return *it;
}

const Account& Accounts::findAccount(const QString& name) const
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const Account& a) { 
            return a.name == name; 
        });
    Q_ASSERT(it != std::end(accounts_));

    return *it;
}

void Accounts::delAccount(std::size_t index)
{
    Q_ASSERT(index < accounts_.size());

    beginRemoveRows(QModelIndex(), index, index);

    auto key = accounts_[index].name;
    auto it  = accounts_.begin();
    it += index;

    auto id = it->id;
    if (id == mainAccount_)
        mainAccount_ = 0;
    else if (id == fillAccount_)
        fillAccount_ = 0;

    g_engine->delAccount(*it);

    accounts_.erase(it);

    endRemoveRows();

    emit accountsUpdated();
}

void Accounts::setAccount(const Account& acc)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [&](const Account& maybe) {
            return maybe.id == acc.id;
        });

    if (it == std::end(accounts_))
    {
        DEBUG(str("Insert account '_1' (_2) ", acc.name, acc.id));        

        beginInsertRows(QModelIndex(), accounts_.size(), accounts_.size());
        accounts_.push_back(acc);
        endInsertRows();

        // first account to be added automatically becomes the main account.
        if (accounts_.size() == 1)
            mainAccount_ = acc.id;
    }
    else
    {
        DEBUG(str("Set account '_1' (_2) ", acc.name, acc.id));

        const auto old_key = it->name;
        const auto new_key = acc.name;
        //if (old_key != new_key)
        //    store.del(old_key);

        *it = acc;
        const auto pos = std::distance(std::begin(accounts_), it);
        const auto first = index(pos, 0);
        const auto last  = index(pos, 0);
        emit dataChanged(first, last);
    }

    g_engine->setAccount(acc);

    emit accountsUpdated();
}

void Accounts::setMainAccount(quint32 id)
{
    // if main account is set to 0 it means that there's
    // no specific main account set and each time
    // when a new download is started we should ask the
    // user about which one he wants to use.
    if (id == 0)
    {
        mainAccount_ = 0;
        return;
    }

    auto it =std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const Account& a) {
            return a.id == id;
        });

    Q_ASSERT(it != std::end(accounts_));

    mainAccount_ = id;

    DEBUG(str("Main account set to _1", it->name));
}

void Accounts::setFillAccount(quint32 id)
{
    // if fill account is set to 0 it means 
    // that there's no fill account to be used.
    if (id == 0)
    {
        fillAccount_ = 0;
        g_engine->setFillAccount(0);
        return;
    }
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const Account& a) {
            return a.id == id;
        });    

    Q_ASSERT(it != std::end(accounts_));

    fillAccount_ = id;
    g_engine->setFillAccount(id);

    DEBUG(str("Fill account set to _1", it->name));
}

bool Accounts::saveState(Settings& store) const
{
    QStringList list;

    for (const auto& acc : accounts_)
    {
        QString key = acc.name;
        store.set(key, "username", acc.username);
        store.set(key, "password", acc.password);
        store.set(key, "general_host", acc.general_host);
        store.set(key, "general_port", acc.general_port);
        store.set(key, "secure_host", acc.secure_host);
        store.set(key, "secure_port", acc.secure_port);        
        store.set(key, "enable_general_server", acc.enable_general_server);
        store.set(key, "enable_secure_server", acc.enable_secure_server);        
        store.set(key, "enable_pipelining", acc.enable_pipelining);
        store.set(key, "enable_compression", acc.enable_compression);
        store.set(key, "enable_login", acc.enable_login);
        store.set(key, "quota_spent", acc.quota_spent);
        store.set(key, "quota_avail", acc.quota_avail);
        store.set(key, "downloads_this_month", acc.downloads_this_month);
        store.set(key, "downloads_all_time", acc.downloads_all_time);
        store.set(key, "quota_type", (int)acc.quota_type);
        store.set(key, "max_connections", acc.max_connections);
        store.set(key, "last_use_date", acc.last_use_date);
        store.set(key, "subscriptions", acc.subscriptions);
        list.append(key);
    }
    store.set("accounts", "list", list);

    if (mainAccount_)
    {
        const auto& main = findAccount(mainAccount_);
        store.set("accounts", "main", main.name);
    }
    else
    {
        store.set("accounts", "main", "");
    }
    if (fillAccount_)
    {
        const auto& fill = findAccount(fillAccount_);
        store.set("accounts", "fill", fill.name);
    }
    else
    {
        store.set("accounts", "fill", "");
    }
    return true;
}

void Accounts::loadState(Settings& store)
{
    QStringList list = store.get("accounts", "list").toStringList();

    for (const auto& key : list)
    {
        Account acc;
        acc.id                    = CurrentAccountId++; //(quint32)store.get(key, "id").toInt();
        acc.name                  = key;
        acc.username              = store.get(key, "username").toString();
        acc.password              = store.get(key, "password").toString();
        acc.general_host          = store.get(key, "general_host").toString();
        acc.general_port          = store.get(key, "general_port").toInt();
        acc.secure_host           = store.get(key, "secure_host").toString();
        acc.secure_port           = store.get(key, "secure_port").toInt();        
        acc.enable_general_server = store.get(key, "enable_general_server").toBool();
        acc.enable_secure_server  = store.get(key, "enable_secure_server").toBool();
        acc.enable_compression    = store.get(key, "enable_compression").toBool();
        acc.enable_pipelining     = store.get(key, "enable_pipelining").toBool();
        acc.enable_login          = store.get(key, "enable_login").toBool();
        acc.quota_spent           = store.get(key, "quota_spent", quint64(0));
        acc.quota_avail           = store.get(key, "quota_avail", quint64(0));        
        acc.downloads_all_time    = store.get(key, "downloads_all_time", quint64(0));
        acc.downloads_this_month  = store.get(key, "downloads_this_month", quint64(0));        
        acc.quota_type            = (Account::quota)store.get(key, "quota_type").toInt();
        acc.max_connections       = store.get(key, "max_connections").toInt();
        acc.last_use_date         = store.get(key, "last_use_date").toDate();
        acc.subscriptions         = store.get(key, "subscriptions").toStringList();
        accounts_.push_back(acc);
        DEBUG(str("Account loaded _1", acc.name));

        g_engine->setAccount(acc);
    }

    const auto& main = store.get("accounts", "main", "");
    const auto& fill = store.get("accounts", "fill", "");
    if (!main.isEmpty())
    {
        const auto& m = findAccount(main);
        mainAccount_ = m.id;
        DEBUG(str("Main Account is _1", main));
    }

    if (!fill.isEmpty())
    {
        const auto& m = findAccount(fill);
        fillAccount_  = m.id;
        g_engine->setFillAccount(fillAccount_);

        DEBUG(str("Fill Account is _1", fill));
    }

    QAbstractItemModel::reset();
}

int Accounts::rowCount(const QModelIndex&) const 
{
    return accounts_.size();
}

QVariant Accounts::data(const QModelIndex& index, int role) const
{

    if (role == Qt::DisplayRole)
    {
        const auto& acc = accounts_[index.row()];
        return acc.name;
    }
    else if (role == Qt::FontRole)
    {
        QFont bold;
        bold.setBold(true);
        return bold;
    }
    else if (role == Qt::DecorationRole)
    {
        return QIcon("icons:ico_account.png");
    }
    return QVariant();
}

Accounts* g_accounts;

} // app
