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

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <memory>
#include <atomic>

namespace app
{
    class Settings;

    class SmtpTask : public QObject
    {
        Q_OBJECT

    public:
        using Error = ::SmtpClient::SmtpError;

        void cancel();

        Error getError() const
        {
            return mError;
        }

    signals:
        void done();

    private:
        bool mSSL = true;
        unsigned mPort = 465;
        QString mHost;
        QString mUsername;
        QString mPassword;
        QString mRecipient;
        QString mSubject;
        QString mMessage;
    private:
        std::atomic<Error> mError;
        std::atomic<bool>  mCancelled;
    private:
        void signal(Error result);
        Error run();
        friend class SmtpClient;
    };

    class SmtpClient  : public QObject
    {
        Q_OBJECT

    public:
        using Error = ::SmtpClient::SmtpError;

        SmtpClient();
       ~SmtpClient();

        void startup();

        void startShutdown();

        void shutdown();

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

        struct Email {
            QString subject;
            QString message;
            QString recipient;
            unsigned port;
            QString host;
            QString username;
            QString password;
            bool ssl;
        };

        std::shared_ptr<SmtpTask> sendEmailNow(const Email& email);

    signals:
        void shutdownComplete();

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
        // unfortunately the third_party/smtpclient is a simple
        // blocking code so in order to maintain UI responsiveness
        // we put the actual work in the background thread.
        // note that this doesn't fully solve the problem since
        // the Qt's blocking socket api doesn't provide cancellation.
        // Eventually the reaĺ solution would be to refactor the
        // third_party/smtpclient to be a non-blocking implementation.
        std::condition_variable mQueueCondition;
        std::mutex  mQueueMutex;
        std::unique_ptr<std::thread> mMailThread;
        std::queue<std::shared_ptr<SmtpTask>> mTaskQueue;
        bool mRunThread = true;

        void mailThreadMain();

    private:
        SmtpClient(const SmtpClient&) = delete;
        SmtpClient& operator=(const SmtpClient&) = delete;
    };

    extern SmtpClient* g_smtp;
} // namespace
