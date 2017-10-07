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

#include "debug.h"
#include "smtpclient.h"
#include "settings.h"
#include "eventlog.h"

namespace app
{

SmtpClient::SmtpClient()
{
    DEBUG("SmtpClient created");
}

SmtpClient::~SmtpClient()
{
    DEBUG("SmtpClient destroyed");
}

SmtpClient::Error SmtpClient::sendEmailNow(const QString& subject, const QString& message)
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
    text.setText(message);

    ::MimeMessage mime;
    mime.setSender(new ::EmailAddress(mRecipient, NEWSFLASH_TITLE));
    mime.addRecipient(new ::EmailAddress(mRecipient, NEWSFLASH_TITLE));
    mime.setSubject(subject);
    mime.addPart(&text);

    if (auto error = smtp.connectToHost())
        return error;
    if (auto error = smtp.loginToServer())
        return error;
    if (auto error = smtp.sendMail(mime))
        return error;
    smtp.quit();

    return SmtpClient::Error::None;
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

    if (const auto error = sendEmailNow(subject, message))
    {
        ERROR("Failed to send email to %1", mRecipient);
    }
}

} // namespace

