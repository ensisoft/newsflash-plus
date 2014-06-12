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

#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/format.h>
#include <newsflash/sdk/datastore.h>
#include <newsflash/sdk/message.h>
#include <newsflash/sdk/message_account.h>
#include <ctime>
#include <algorithm>

#include "accounts.h"


using sdk::str;
using sdk::str_a;

namespace app
{

accounts::accounts()
{
    sdk::listen<sdk::msg_get_account>(this, "accounts");
    sdk::listen<sdk::msg_file_complete>(this, "accounts");
    DEBUG("accounts created");
}

accounts::~accounts()
{
    sdk::remove_listener(this);
    DEBUG("accounts deleted");
}

account accounts::suggest() const
{
    int index = 1;
    for (;;)
    {
        const auto& suggestion = str("My Usenet _1", index);

        const auto& it = std::find_if(std::begin(accounts_), std::end(accounts_),
            [&](const account& acc) {
                return acc.name() == suggestion;
            });
        if (it == std::end(accounts_))
        {
            return account { suggestion };
        }
        ++index;
    }
}

const 
account& accounts::get(std::size_t index) const
{
    return accounts_[index];
}

account& accounts::get(std::size_t index) 
{
    return accounts_[index];
}

void accounts::del(std::size_t index)
{
    Q_ASSERT(index < accounts_.size());

    const auto acc = get(index);
    const auto aid = acc.id();

    beginRemoveRows(QModelIndex(), index, index);
    accounts_.removeAt(index);
    endRemoveRows();

    sdk::send(sdk::msg_del_account{aid}, "accounts");
}

void accounts::set(const account& acc)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [&](const account& maybe) {
            return maybe.id() == acc.id();
        });

    if (it == std::end(accounts_))
    {
        DEBUG(str("Insert account _1", acc.id()));

        beginInsertRows(QModelIndex(), accounts_.size(), accounts_.size());
        accounts_.push_back(acc);
        endInsertRows();
    }
    else
    {
        DEBUG(str("Set account _1", acc.id()));

        *it = acc;
        const auto pos = std::distance(std::begin(accounts_), it);
        const auto first = index(pos, 0);
        const auto last  = index(pos, 0);
        emit dataChanged(first, last);
    }

    sdk::msg_set_account msg {};
    msg.id   = acc.id();
    msg.name = acc.name();
    sdk::send(msg, "accounts");
}

void accounts::save(sdk::datastore& datastore) const
{
    QStringList list;

    for (const auto& acc : accounts_)
    {
        const auto& key = QString::number(acc.id());

        acc.save(key, datastore);
        list.append(key);
    }

    datastore.set("accounts", "list", list);
}

void accounts::load(const sdk::datastore& datastore)
{
    QStringList list =  datastore.get("accounts", "list").toStringList();

    for (const auto& key : list)
    {
        account acc(key, datastore);

        DEBUG(str("Account loaded _1, _2", acc.name(), acc.id()));

        accounts_.push_back(acc);


    }
    QAbstractItemModel::reset();
}

QAbstractItemModel* accounts::view() 
{
    return this;
}

QString accounts::name() const 
{
    return "accounts";
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
        return acc.name();
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

void accounts::on_message(const char* sender, sdk::msg_get_account& msg)
{
    msg.success = false;

    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [&](const account& acc) {
            return acc.id() == msg.id;
        });
    if (it == std::end(accounts_))
        return;

    const auto& acc = *it;
    msg.success  = true;
    msg.name     = acc.name();
    msg.password = acc.password();
    msg.username = acc.username();
        // etc.
}

void accounts::on_message(const char* sender, sdk::msg_file_complete& msg)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [&](const account& acc) {
            return acc.id() == msg.account;
        });
    if (it == std::end(accounts_))
        return;

    auto& acc = *it;

    acc.on_message(sender, msg);
}

} // app
