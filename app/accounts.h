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

#pragma once

#include <newsflash/config.h>

#include <newsflash/sdk/model.h>

#include <newsflash/warnpush.h>
#  include <QAbstractListModel>
#  include <QDateTime>
#  include <QString>
#  include <QList>
#  include <QObject>
#include <newsflash/warnpop.h>

namespace app
{
    class accounts : public sdk::model, public QAbstractListModel
    {
    public:
        enum class quota {
            fixed, monthly
        };

        struct account {
            quint32 id;
            QString name;
            QString username;
            QString password;
            QString general_host;
            quint16 general_port;        
            QString secure_host;        
            quint16 secure_port;
            bool enable_general_server;
            bool enable_secure_server;
            bool enable_login;
            bool enable_compression;
            bool enable_pipelining;
            bool enable_quota;
            int  connections;
            quint64 quota_avail; // available quota bytes
            quint64 quota_spent;  // how much is spent of the quota
            quint64 downloads_this_month;
            quint64 downloads_all_time;
            quota quota_type;
        };

        // persist accounts into datastore
        void save(sdk::datastore& datastore) const;

        // retrieve accounts from datastore
        void load(const sdk::datastore& datastore);

        // suggest a new account object
        account suggest() const;

        // get account at index
        const 
        account& get(std::size_t index) const;

        account& get(std::size_t index);
        
        // delete the account at index 
        void del(std::size_t index);

        // insert or modify an account.
        // if account already exists it's modified otherwise it's inserted
        void set(const accounts::account& acc);

        // model impl
        virtual QAbstractItemModel* view() override;



        // AbstractListModel impl
        virtual int rowCount(const QModelIndex&) const override;

        // AbstractListMode impl
        virtual QVariant data(const QModelIndex&, int role) const override;
    private:
        QList<account> accounts_;
    };

} // app
   