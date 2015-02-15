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
#  include <QObject>
#include <newsflash/warnpop.h>

#include <memory>
#include <vector>
#include "account.h"

namespace app
{
    class Settings;

    // Accounts model stores and manages the accounts configured
    // in the application.
    class Accounts : public QAbstractListModel
    {
        Q_OBJECT

    public:
        // different quota types
        enum class Quota {
            // quota is not being used
            none, 

            // quota is a fixed block for example 25gb 
            // but without any time limit
            fixed, 

            // quota is per month
            monthly
        };

        struct Account {
            quint32 id;
            QString name;
            QString username;
            QString password;
            QString generalHost;
            quint16 generalPort;        
            QString secureHost;
            quint16 securePort;
            bool enableGeneralServer;
            bool enableSecureServer;
            bool enableCompression;
            bool enablePipelining;
            bool enableLogin;
            quint64 quotaSpent;
            quint64 quotaAvail;
            quint64 downloadsThisMonth;
            quint64 downloadsAllTime;
            Quota quotaType;
            int maxConnections;
            QDate lastUseDate;
            QStringList subscriptions;            
        };

        Accounts();
       ~Accounts();

        // suggest a new account object
        Account suggestAccount() const;

        // get account at index
        const 
        Account& getAccount(std::size_t index) const;
        Account& getAccount(std::size_t index);
        
        // get the currently configured fill account.
        // if fill account is not set returns a nullptr
        const Account* getFillAccount() const;

        // get the current main account. 
        const Account* getMainAccount() const;

        const 
        Account& findAccount(quint32 accountId) const;
        Account& findAccount(quint32 accountId);

        const 
        Account& findAccount(const QString& name) const;
        Account& findAccount(const QString& name);

        // delete the account at index 
        void delAccount(std::size_t index);

        // insert or modify an account.
        // if account already exists it's modified otherwise it's inserted
        void setAccount(const Account& acc);

        // set the account identified by id to be main account.
        // main account is the primary account used for downloading
        // as opposed to fill account which is only used for 
        // downloading the missing pieces. if main account is not
        // and there are multiple accounts then the user should be asked
        // for the account that they want to use.
        void setMainAccount(quint32 id);

        // set the account by id to be the current fill account.
        // fill account is used to download pieces missing on the
        // primary/main download account.
        void setFillAccount(quint32 id);

        // persist accounts into datastore
        bool saveState(Settings& store) const;

        // retrieve accounts from datastore
        void loadState(Settings& store); 

        // get the number of accounts.
        std::size_t numAccounts() const
        { 
            return accounts_.size(); 
        }

        // AbstractListModel impl
        virtual int rowCount(const QModelIndex&) const override;

        // AbstractListMode impl
        virtual QVariant data(const QModelIndex&, int role) const override;

    signals:
        void accountsUpdated();

    private:
        std::vector<Account> accounts_;
        quint32 mainAccount_;
        quint32 fillAccount_;
    };

    extern Accounts* g_accounts;

} // app
   