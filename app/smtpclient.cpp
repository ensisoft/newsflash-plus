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

#define LOGTAG "smtp"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <third_party/smtpclient/smtpclient.h>
#  include <third_party/smtpclient/mimetext.h>
#  include <third_party/smtpclient/mimemessage.h>
#include "newsflash/warnpop.h"

#include <functional>

#include "debug.h"
#include "smtpclient.h"
#include "settings.h"
#include "eventlog.h"

namespace app
{

SmtpClient* g_smtp;

void SmtpTask::cancel()
{
    // we don't actually support this currently properly.
    mCancelled = true;
}

SmtpTask::Error SmtpTask::run()
{
    ::SmtpClient smtp;
    smtp.setHost(mHost);
    smtp.setPort(mPort);
    smtp.setPassword(mPassword);
    smtp.setUsername(mUsername);
    smtp.setConnectionType(mSSL
        ? ::SmtpClient::SslConnection
        : ::SmtpClient::TcpConnection);

    ::MimeText text;
    text.setText(mMessage);

    ::MimeMessage mime;
    mime.setSender(new ::EmailAddress(mRecipient, NEWSFLASH_TITLE));
    mime.addRecipient(new ::EmailAddress(mRecipient, NEWSFLASH_TITLE));
    mime.setSubject(mSubject);
    mime.addPart(&text);

    auto error = smtp.connectToHost();
    if (error || mCancelled)
        return error;

    error = smtp.loginToServer();
    if (error || mCancelled)
        return error;

    error = smtp.sendMail(mime);
    if (error || mCancelled)
        return error;
    smtp.quit();
    return error;
}

void SmtpTask::signal(SmtpTask::Error result)
{
    mError = result;

    emit done();
}

SmtpClient::SmtpClient()
{
    DEBUG("SmtpClient created");
}

SmtpClient::~SmtpClient()
{
    DEBUG("SmtpClient destroyed");
}

void SmtpClient::startup()
{
    std::lock_guard<std::mutex> lock(mQueueMutex);
    mRunThread  = true;
    mMailThread = std::make_unique<std::thread>(std::bind(&SmtpClient::mailThreadMain, this));
    DEBUG("Started SmtpClient thread");
}

void SmtpClient::startShutdown()
{
   std::lock_guard<std::mutex> lock(mQueueMutex);
   mRunThread = false;
   mQueueCondition.notify_one();
}

void SmtpClient::shutdown()
{
    mMailThread->join();
    mMailThread.reset();
    DEBUG("Shutdown (joined) SmtpClient thread");
}

std::shared_ptr<SmtpTask> SmtpClient::sendEmailNow(const Email& email)
{
    auto sendMail = std::make_shared<SmtpTask>();
    sendMail->mSSL  = email.ssl;
    sendMail->mPort = email.port;
    sendMail->mHost = email.host;
    sendMail->mUsername = email.username;
    sendMail->mPassword = email.password;
    sendMail->mRecipient = email.recipient;
    sendMail->mSubject = email.subject;
    sendMail->mMessage = email.message;

    std::lock_guard<std::mutex> lock(mQueueMutex);
    mTaskQueue.push(sendMail);
    mQueueCondition.notify_one();

    return sendMail;
}

void SmtpClient::saveState(Settings& settings) const
{
    settings.set("smtpclient", "enabled", mEnabled);
    settings.set("smtpclient", "host", mHost);
    settings.set("smtpclient", "port", mPort);
    settings.set("smtpclient", "ssl",  mSSL);
    settings.set("smtpclient", "username", mUsername);
    settings.set("smtpclient", "password", mPassword);
    settings.set("smtpclient", "recipient", mRecipient);
}

void SmtpClient::loadState(const Settings& settings)
{
    mEnabled = settings.get("smtpclient", "enabled").toBool();
    mHost = settings.get("smtpclient", "host").toString();
    mPort = settings.get("smtpclient", "port").toInt();
    mSSL  = settings.get("smtpclient", "ssl").toBool();
    mUsername  = settings.get("smtpclient", "username").toString();
    mPassword  = settings.get("smtpclient", "password").toString();
    mRecipient = settings.get("smtpclient", "recipient").toString();
}

void SmtpClient::sendEmail(const QString& subject, const QString& message)
{
    if (!mEnabled)
        return;

    auto sendMail = std::make_shared<SmtpTask>();
    sendMail->mSSL  = mSSL,
    sendMail->mPort = mPort;
    sendMail->mHost = mHost;
    sendMail->mUsername = mUsername;
    sendMail->mPassword = mPassword;
    sendMail->mRecipient = mRecipient;
    sendMail->mSubject = subject;
    sendMail->mMessage = message;

    {
        std::lock_guard<std::mutex> lock(mQueueMutex);
        mTaskQueue.push(std::move(sendMail));
        mQueueCondition.notify_one();
    }
}

void SmtpClient::mailThreadMain()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock(mQueueMutex);
        if (!mRunThread)
            break;
        else if (mTaskQueue.empty())
        {
            mQueueCondition.wait(lock);
            // handle spurious wakeup
            continue;
        }

        std::shared_ptr<SmtpTask> task = std::move(mTaskQueue.front());
        mTaskQueue.pop();
        lock.unlock();

        DEBUG("SmtpClient thread running task");

        const auto result = task->run();
        task->signal(result);
    }

    emit shutdownComplete();
}

} // namespace

