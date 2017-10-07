// Copyright (c) 2010-2017 Sami Väisänen, Ensisoft
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
#  include <QObject>
#  include <QString>
#  include <third_party/smtpclient/smtpclient.h>
#include "newsflash/warnpop.h"


namespace app
{
    class Settings;

    class SmtpClient  : public QObject
    {
        Q_OBJECT

    public:
        using Error = ::SmtpClient::SmtpError;

        SmtpClient();
       ~SmtpClient();

        void setRecipient(const QString& email)
        { mRecipient = email; }

        void setUsername(const QString& user)
        { mUsername = user; }

        void setPassword(const QString& pass)
        { mPassword = pass; }

        void setPort(unsigned port)
        { mPort = port; }

        void setHost(const QString& host)
        { mHost = host; }

        void setSSL(bool onOff)
        { mSSL = onOff; }

        void setEnabled(bool onOff)
        { mEnabled = onOff; }

        QString username() const
        { return mUsername; }

        QString password() const
        { return mPassword; }

        QString host() const
        { return mHost; }

        unsigned port() const
        { return mPort; }

        bool ssl() const
        { return mSSL; }

        bool enabled() const
        { return mEnabled; }

        QString recipient() const
        { return mRecipient; }

        void saveState(Settings& settings) const;
        void loadState(const Settings& settings);

        Error sendEmailNow(const QString& subject, const QString& message);

    public slots:
        void sendEmail(const QString& subject, const QString& message);

    private:
        bool mEnabled = false;
        QString mHost;
        unsigned mPort = 465;
        bool mSSL = true;
        QString mUsername;
        QString mPassword;
    private:
        QString mRecipient;
    private:
        SmtpClient(const SmtpClient&) = delete;
        SmtpClient& operator=(const SmtpClient&) = delete;
    };
} // namespace
