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

#define LOGTAG "net"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkReply>
#  include <QtNetwork/QNetworkRequest>
#include <newsflash/warnpop.h>

#include <stdexcept>

#include "netman.h"
#include "format.h"
#include "debug.h"
#include "eventlog.h"

// Notes about the implementation...
// Qt 4.5.3 QNetworkAccessManager has a problem with network down situations.
// If a request has already been succesfully made before "connection" is established and
// the network goes down during/before a subsequent request there is no way to get a 
// timeout signal from the network access manager. However on "first" request timeout works
// as expected.
//
// I tried to workaround this problem by using a timer per request but that turns out to create
// more problems (more Qt bugs). One problem is that calling QNetworkReply::abort() on timer 
// sometimes leads to an assert in Qt code. I.e. if the first request was aborted and later 
// QNetworkAccessManager timeouts the same request an assert is fired. Simply deleting (deleteLater())
// the request doesn't work either because then it still seems to be in Qt queues and on exiting the 
// application application gets stuck in pthread_cond_wait when destruction QApplication.

namespace {

const char* nstr(QNetworkReply::NetworkError err)
{
    switch (err)
    {
        case QNetworkReply::ConnectionRefusedError: return "connection refused";
        case QNetworkReply::RemoteHostClosedError: return "remote host closed";
        case QNetworkReply::HostNotFoundError: return "host not found";
        case QNetworkReply::TimeoutError: return "timeout";
        case QNetworkReply::OperationCanceledError: return "operation canceled";
        case QNetworkReply::SslHandshakeFailedError: return "SSL handshake failed";
        case QNetworkReply::TemporaryNetworkFailureError: return "temporary network failure";
        case QNetworkReply::ProxyConnectionRefusedError: return "proxy connection refused";
        case QNetworkReply::ProxyConnectionClosedError: return "proxy connection closed";
        case QNetworkReply::ProxyNotFoundError: return "proxy not found";
        case QNetworkReply::ProxyAuthenticationRequiredError: return "proxy authentication required";
        case QNetworkReply::ProxyTimeoutError: return "proxy timeout";
        case QNetworkReply::ContentOperationNotPermittedError: return "content operation not permitted";
        case QNetworkReply::ContentNotFoundError: return "content not found";
        case QNetworkReply::ContentReSendError: return "content re-send";
        case QNetworkReply::AuthenticationRequiredError: return "authentication required";
        case QNetworkReply::ProtocolUnknownError: return "unknown protocol";
        case QNetworkReply::ProtocolInvalidOperationError: return "invalid protocol operation";
        case QNetworkReply::UnknownNetworkError: return "unknown network";
        case QNetworkReply::UnknownProxyError: return "unknown proxy";
        case QNetworkReply::UnknownContentError: return "unknown content";
        case QNetworkReply::ContentAccessDenied: return "access denied";
        case QNetworkReply::ProtocolFailure: return "protocol failure";
        case QNetworkReply::NoError: return "success";
    }
    Q_ASSERT(!"wat");
    return nullptr;
}

} // namespace

namespace app
{

netman::netman() : submission_id_(1)
{
    DEBUG("netman created");

    QObject::connect(&qnam_, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(finished(QNetworkReply*)));
    QObject::connect(&timer_, SIGNAL(timeout()),
        this, SLOT(timeout()));
}

netman::~netman()
{
    if (!submissions_.empty())
    {
        timer_.stop();

        DEBUG(str("netman has _1 pending submissions...", submissions_.size()));

        for (auto it = submissions_.begin(); it != submissions_.end(); ++it)
        {
            auto& submission = *it;
            if (submission.reply)
            {
                submission.cancel = true;
                submission.reply->abort();
            }
            it = submissions_.erase(it);
        }
    }

    DEBUG("netman destroyed");
}

void netman::submit(callback completion, QUrl url)
{
    submission s;
    s.id     = submission_id_;
    s.ticks  = 0;
    s.reply  = nullptr;
    s.url    = std::move(url);
    s.cancel = false;
    s.timeout = false;
    submissions_.push_back(s);

    if (submissions_.size() == 1)
    {
        submit_next();
        timer_.setInterval(1000);
        timer_.start();
    }

    DEBUG(str("New submission _1", submission_id_));

    ++submission_id_;
}

void netman::cancel()
{
    for (auto it = submissions_.begin(); it != submissions_.end(); ++it)
    {
        auto& submission = *it;
        if (submission.reply)
        {
            submission.cancel = true;
            submission.reply->abort();
        }

        DEBUG(str("Canceled submission _1", submission.id));

        it = submissions_.erase(it);        
    }
}

void netman::finished(QNetworkReply* reply)
{
    auto it = std::find_if(std::begin(submissions_), std::end(submissions_),
        [=](const submission& s) {
            return s.reply == reply;
        });
    Q_ASSERT(it != std::end(submissions_));

    auto& submit   = *it;
    const auto err = reply->error();
    const auto url = reply->url();

    DEBUG(str("Got network reply for submission _1, _2", submit.id, nstr(err)));

    if (err != QNetworkReply::NoError)
    {
        ERROR(str("Network request _1 failed, _2", url, nstr(err)));
    }

    if (submit.cancel)
    {
        DEBUG("Submit was canceled. Ignoring...");
    }
    else
    {
        submit.cb(*reply);
        if (!submit.timeout)
            submissions_.erase(it);
    }

    reply->deleteLater();

    DEBUG(str("Pending submissions _1", submissions_.size()));
}

void netman::timeout()
{
    submit_next();
    update_ticks();

    if (submissions_.empty())
        timer_.stop();
}

void netman::submit_next()
{
    auto it = std::find_if(std::begin(submissions_), std::end(submissions_),
        [](const submission& s) {
            return s.reply == nullptr;
        });
    if (it == std::end(submissions_))
        return;

    auto& submit = *it;

    QNetworkRequest request;
    request.setRawHeader("User-Agent", "NewsflashPlus");
    request.setUrl(submit.url);
    submit.reply = qnam_.get(request);
    if (!submit.reply)
        throw std::runtime_error("network request failed");
}

void netman::update_ticks()
{
    const auto timeout_ticks = 30;

    for (auto it = submissions_.begin(); it != submissions_.end(); ++it)
    {
        auto& submission = *it;
        if (!submission.reply)
            continue;

        if (++submission.ticks == timeout_ticks)
        {
            submission.timeout = true;
            submission.reply->abort();
        }
        DEBUG(str("Timedout submission _1", submission.id));

        submissions_.erase(it);
    }

}

} // namespace