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

#include <newsflash/warnpush.h>
#  include <QAbstractListModel>
#  include <QDateTime>
#  include <QString>
#  include <QList>
#  include <QObject>
#include <newsflash/warnpop.h>

namespace app
{
    class valuestore;

    class accounts : public QAbstractListModel
    {
    public:
        struct server {
            QString host;
            qint16  port;
            bool    enabled;
        };

        struct account {
            quint32 id;
            QString name;
            QString user;
            QString pass;
            QString path;
            server  general;
            server  secure;

            bool requires_login;
            bool enable_compression;
            bool enable_pipelining;
            int maxconn;
        };

        // persist accounts into valuestore
        void persist(app::valuestore& valuestore) const;

        // retrieve accounts from valuestore
        void retrieve(const app::valuestore& valuestore);

        // create a new account object
        void suggest(account& ac) const;

        // insert or modify an existing account.
        void set(const accounts::account& acc);

        const 
        account& get(std::size_t i) const;

        account& get(std::size_t i);

        int rowCount(const QModelIndex&) const override;

        QVariant data(const QModelIndex&, int role) const override;

    private:
        QList<account> accounts_;
    };

} // app
 