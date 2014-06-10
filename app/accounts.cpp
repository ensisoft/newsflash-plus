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
#  include <QtGui/QFont>
#  include <QtGui/QIcon>
#  include <QDir>
#  include <QStringList>
#include <newsflash/warnpop.h>

#include <newsflash/sdk/format.h>
#include <newsflash/sdk/datastore.h>

#include <ctime>
#include <algorithm>

#include "accounts.h"
#include "eventlog.h"

using sdk::str;
using sdk::str_a;

namespace app
{

void accounts::save(sdk::datastore& datastore) const
{

#define SAVE(x) \
    datastore.set(key, #x, acc.x)

    QStringList accounts;
    for (const auto& acc : accounts_)
    {
        const auto& key = acc.name;

        SAVE(id);
        SAVE(name);
        SAVE(username);
        SAVE(password);
        SAVE(general_host);
        SAVE(general_port);
        SAVE(secure_host);
        SAVE(secure_port);
        SAVE(enable_general_server);
        SAVE(enable_secure_server);
        SAVE(enable_login);
        SAVE(enable_compression);
        SAVE(enable_pipelining);
        SAVE(enable_quota);
        SAVE(connections);
        SAVE(quota_avail);
        SAVE(quota_spent);
        SAVE(downloads_this_month);
        SAVE(downloads_all_time);

        datastore.set(key, "quota_type", (int)acc.quota_type);
        

        accounts.append(key);
    }
    datastore.set("accounts", "list", accounts);

#undef SAVE
}

void accounts::load(const sdk::datastore& datastore)
{
    // we use the template version of the get method to resolve
    // the type of the account member. however in order for 
    // automatic type deduction to kick in we must provide
    // a default value. for this we use the original value 
    // which then gets overwritten.
#define LOAD(x) \
    acc.x = datastore.get(key, #x, acc.x)

    const auto& accounts = datastore.get("accounts", "list").toStringList();
    for (const auto& name : accounts)
    {
        const auto& key = name;
        account acc = {};

        LOAD(id);
        LOAD(name);
        LOAD(username);
        LOAD(password);
        LOAD(general_host);
        LOAD(general_port);
        LOAD(secure_host);
        LOAD(secure_port);
        LOAD(enable_general_server);
        LOAD(enable_secure_server);
        LOAD(enable_login);
        LOAD(enable_compression);
        LOAD(enable_pipelining);
        LOAD(enable_quota);
        LOAD(connections);
        LOAD(quota_avail);
        LOAD(quota_spent);
        LOAD(downloads_this_month);
        LOAD(downloads_all_time);

        acc.quota_type = (accounts::quota)datastore.get(key, "quota_type").toInt();

        accounts_.append(acc);
    }
    QAbstractItemModel::reset();
    
#undef LOAD
}

accounts::account accounts::suggest() const
{
    account acc = {};
    acc.id                    = (quint32)std::time(nullptr);
    acc.enable_general_server = false;
    acc.enable_secure_server  = false;
    acc.general_port          = 119;
    acc.secure_port           = 563;
    acc.enable_login          = false;
    acc.enable_compression    = false;
    acc.enable_pipelining     = false;
    acc.connections           = 5;

    int index = 1;
    for (;;)
    {
        const auto& suggestion = str("My Usenet _1", index);

        const auto& it = std::find_if(std::begin(accounts_), std::end(accounts_),
            [&](const account& acc) {
                return acc.name == suggestion;
            });
        if (it == std::end(accounts_))
        {
            acc.name = suggestion;
            break;
        }
        ++index;
    }
    return acc;
}

void accounts::set(const accounts::account& acc)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [&](const account& maybe) {
            return maybe.id == acc.id;
        });

    if (it == std::end(accounts_))
    {
        beginInsertRows(QModelIndex(), accounts_.size(), accounts_.size());
        accounts_.push_back(acc);
        endInsertRows();
    }
    else
    {
        *it = acc;
        const auto pos = std::distance(std::begin(accounts_), it);
        const auto first = index(pos, 0);
        const auto last  = index(pos, 0);
        emit dataChanged(first, last);
    }
}

QAbstractItemModel* accounts::view() 
{
    return this;
}

const 
accounts::account& accounts::get(std::size_t index) const
{
    Q_ASSERT(index < accounts_.size());

    return accounts_[index];
}

accounts::account& accounts::get(std::size_t index)
{
    Q_ASSERT(index < accounts_.size());

    return accounts_[index];
}

void accounts::del(std::size_t index)
{
    Q_ASSERT(index < accounts_.size());

    beginRemoveRows(QModelIndex(), index, index);
    accounts_.removeAt(index);
    endRemoveRows();
}

int accounts::rowCount(const QModelIndex&) const 
{
    return accounts_.size();
}

QVariant accounts::data(const QModelIndex& index, int role) const
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
        return QIcon(":/resource/16x16_ico_png/ico_account.png");
    }
    return QVariant();
}

} // app
