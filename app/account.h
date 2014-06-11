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

#include <newsflash/sdk/message.h>
#include <newsflash/sdk/message_account.h>
#include <newsflash/sdk/message_file.h>
#include <newsflash/sdk/datastore.h>

#include <newsflash/warnpush.h>
#  include <QDate>
#  include <QtGlobal>
#  include <QString>
#  include <QList>
#include <newsflash/warnpop.h>

#define RW_PROPERTY(type, name, value) \
    private: type m_##name { value }; \
    public:  type name() const { return m_##name; } \
    public:  void name(const type& val) { m_##name = val; }

//#define RO_PROPERTY()

namespace app
{
    class account 
    {
    public:
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

        // public properties
        RW_PROPERTY(quint32, id, 0);
        RW_PROPERTY(QString, name, "");
        RW_PROPERTY(QString, username, "");
        RW_PROPERTY(QString, password, "");
        RW_PROPERTY(QString, general_host, "");
        RW_PROPERTY(quint16, general_port, 119);
        RW_PROPERTY(QString, secure_host, "");
        RW_PROPERTY(quint16, secure_port, 563);
        RW_PROPERTY(bool, enable_general_server, false);
        RW_PROPERTY(bool, enable_secure_server, false);
        RW_PROPERTY(bool, enable_login, false);
        RW_PROPERTY(bool, enable_compression, false);
        RW_PROPERTY(bool, enable_pipelining, false);
        RW_PROPERTY(int, connections, 5);
        RW_PROPERTY(quint64, quota_total, 0);
        RW_PROPERTY(quint64, quota_spent, 0);
        RW_PROPERTY(quint64, downloads_this_month, 0);
        RW_PROPERTY(quint64, downloads_all_time, 0);
        RW_PROPERTY(quota, quota_type, quota::none);

        account(QString name) : m_name(std::move(name))
        {
            m_id = (quint32)std::time(nullptr);
        }

        account(const QString& key, const sdk::datastore& store)
        {
#define LOAD(x) \
    m_##x = store.get(key, #x, m_##x)

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
            LOAD(connections);
            LOAD(quota_total);
            LOAD(quota_spent);
            LOAD(downloads_this_month);
            LOAD(downloads_all_time);     
            m_quota_type = (quota)store.get(m_name, "quota_type").toInt();
#undef LOAD

        }

       ~account()
        {}

        void save(const QString& key, sdk::datastore& store) const
        {
#define SAVE(x) \
    store.set(key, #x, m_##x)

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
            SAVE(connections);
            SAVE(quota_total);
            SAVE(quota_spent);
            SAVE(downloads_this_month);
            SAVE(downloads_all_time);     
            store.set(m_name, "quota_type", (int)m_quota_type);

#undef SAVE
        }

        void reset_month()
        {
            m_downloads_this_month = 0;
        }

        void reset_all_time()
        {
            m_downloads_all_time = 0;
        }

        quint64 quota_avail() const {
            if (m_quota_spent >= m_quota_total)
                return 0;
            return m_quota_total - m_quota_spent;
        }

    public:
        void on_message(const char* sender, sdk::msg_file_complete& msg)
        {
            if (msg.account != m_id)
                return;

            // respond to a file complete, update quotas and download counters

            bool enabled = m_quota_type != quota::none;
            bool monthly = m_quota_type == quota::monthly;

            m_quota_spent += msg.local_size;

            sdk::send(sdk::msg_account_quota_update{
                m_id, 
                m_quota_total,
                m_quota_total - m_quota_spent,
                m_quota_spent, 
                enabled, 
                monthly}, "accounts");

            m_downloads_all_time += msg.local_size;
            m_downloads_this_month += msg.local_size;

            sdk::send(sdk::msg_account_downloads_update{
                m_id,
                m_downloads_all_time,
                m_downloads_this_month}, "accounts");
        }

    private:
    };

} // app
