// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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
#  include <QtGlobal>
#  include <QString>
#  include <QDate>
#  include <QStringList>
#include <newsflash/warnpop.h>

namespace app
{
    struct Account
    {
        // different quota types
        enum class quota {
            // quota is not being used
            none, 

            // quota is a fixed block for example 25gb 
            // but without any time limit
            fixed, 

            // quota is per month
            monthly
        };

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
        bool enable_compression;
        bool enable_pipelining;
        bool enable_login;
        quint64 quota_spent;
        quint64 quota_avail;
        quint64 downloads_this_month;
        quint64 downloads_all_time;
        quota quota_type;
        int max_connections;
        QDate last_use_date;
        QStringList subscriptions;
    };

} // app
