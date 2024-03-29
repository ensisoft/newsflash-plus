// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QAbstractListModel>
#  include <QDateTime>
#  include <QString>
#  include <QObject>
#include "newsflash/warnpop.h"

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
            quint32 id = 0;
            QString name;
            QString username;
            QString password;
            QString generalHost;
            quint16 generalPort = 119;
            QString secureHost;
            quint16 securePort = 563;
            bool enableGeneralServer = false;
            bool enableSecureServer  = false;
            bool enableCompression   = false;
            bool enablePipelining    = false;
            bool enableLogin         = false;
            quint64 quotaSpent = 0;
            quint64 quotaAvail = 0;
            quint64 downloadsThisMonth = 0;
            quint64 downloadsAllTime = 0;
            Quota quotaType = Quota::none;
            int maxConnections = 5;
            QDate lastUseDate;

            // list of favourite newsgroups
            QStringList favorites;
            QString datapath;
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
        Account* findAccount(quint32 accountId) const;
        Account* findAccount(quint32 accountId);

        const
        Account* findAccount(const QString& name) const;
        Account* findAccount(const QString& name);

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
            return mAccounts.size();
        }

        // AbstractListModel impl
        virtual int rowCount(const QModelIndex&) const override;

        // AbstractListMode impl
        virtual QVariant data(const QModelIndex&, int role) const override;

    signals:
        void accountsUpdated();
        void accountUpdated(const Account* acc);

    private slots:
        void quotaUpdate(std::size_t bytes, std::size_t account);

    private:
        std::vector<Account> mAccounts;
        quint32 mMainAccount = 0;
        quint32 mFillAccount = 0;
    };

    extern Accounts* g_accounts;

} // app
