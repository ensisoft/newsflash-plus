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
#  include <QDir>
#include <newsflash/warnpop.h>

#include <ctime>
#include <algorithm>

#include "accounts.h"
#include "valuestore.h"
#include "eventlog.h"
//#include "format.h"

namespace app
{

void accounts::persist(app::valuestore& valuestore) const
{
    int count = 0;
    for (const auto& acc : accounts_)
    {
  //      const auto& key  = str("account_1", count);
        QString key;
        valuestore.set(key, "name", acc.name);
        valuestore.set(key, "id", acc.id);
        valuestore.set(key, "username", acc.user);
        valuestore.set(key, "password", acc.pass);
        valuestore.set(key, "path", acc.path);
        valuestore.set(key, "general_server_host", acc.general.host);
        valuestore.set(key, "general_server_port", acc.general.port);        
        valuestore.set(key, "general_server_enabled", acc.general.enabled);
        valuestore.set(key, "secure_server_host", acc.secure.host);
        valuestore.set(key, "secure_server_port", acc.secure.port);
        valuestore.set(key, "secure_server_enabled", acc.secure.enabled);
        valuestore.set(key, "requires_login", acc.requires_login);
        valuestore.set(key, "enable_compression", acc.enable_compression);
        valuestore.set(key, "enable_pipelining", acc.enable_pipelining);
    }
    valuestore.set("accounts", "count", count);
}

void accounts::retrieve(const app::valuestore& valuestore)
{
    const int count = valuestore.get("accounts", "count", 0);
    for (int i=0; i<count; ++i)
    {
//        const auto& key = str("account_1", count);
        const QString key;
        account acc;
        acc.id                 = valuestore.get(key, "id").toLongLong();        
        acc.name               = valuestore.get(key, "name").toString();
        acc.user               = valuestore.get(key, "username").toString();
        acc.pass               = valuestore.get(key, "password").toString();
        acc.path               = valuestore.get(key, "path").toString();
        acc.general.host       = valuestore.get(key, "general_server_host").toString();
        acc.general.port       = valuestore.get(key, "general_server_port").toInt();
        acc.general.enabled    = valuestore.get(key, "general_server_enabled").toBool();
        acc.secure.host        = valuestore.get(key, "secure_server_host").toString();
        acc.secure.port        = valuestore.get(key, "secure_server_port").toInt();
        acc.secure.enabled     = valuestore.get(key, "secure_server_enabled").toBool();
        acc.requires_login     = valuestore.get(key, "requires_login").toBool();
        acc.enable_compression = valuestore.get(key, "enable_compression").toBool();
        acc.enable_pipelining  = valuestore.get(key, "enable_pipelining").toBool();
        accounts_.append(acc);
    }
}

void accounts::suggest(account& acc) const
{
    const auto& home = QDir::homePath();
    const auto& data = home + "/Newsflash/Data";

    acc.id                 = (quint32)std::time(nullptr);
    acc.path               = QDir::toNativeSeparators(data);
    acc.general.enabled    = false;
    acc.general.port       = 119;
    acc.secure.port        = 563;
    acc.secure.enabled     = false;
    acc.requires_login     = false;
    acc.enable_compression = false;
    acc.enable_pipelining  = false;
    acc.maxconn            = 5;

    int index = 1;
    for (;;)
    {
//        auto suggestion = str("My Usenet _1", index);
        QString suggestion;
        auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
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
}

void accounts::set(const accounts::account& acc)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [&](const account& maybe) {
            return maybe.id == acc.id;
        });

    std::size_t index = 0;

    if (it == std::end(accounts_))
    {
        accounts_.push_back(acc);
        index = accounts_.size() -1;
    }
    else
    {
        index = std::distance(std::begin(accounts_), it);
        *it = acc;
    }
 //   emit modify_account(index);
}

const
accounts::account& accounts::get(std::size_t i) const
{
    Q_ASSERT(i < accounts_.size());
    return accounts_[i];
}

accounts::account& accounts::get(std::size_t i)
{
    Q_ASSERT(i < accounts_.size());
    return accounts_[i];
}

int accounts::rowCount(const QModelIndex&) const 
{
    return 0;
}

QVariant accounts::data(const QModelIndex&, int role) const
{
    return QVariant();
}

} // app
