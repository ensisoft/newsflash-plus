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

namespace app
{

NetworkManager* g_net;

NetworkManager::NetworkManager()
{
    DEBUG("NetworkManager created");

    QObject::connect(&qnam_, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(finished(QNetworkReply*)));
    QObject::connect(&timer_, SIGNAL(timeout()),
        this, SLOT(timeout()));
}

NetworkManager::~NetworkManager()
{
    if (!submissions_.empty())
    {
        timer_.stop();
        timer_.blockSignals(true);

        DEBUG(str("NetworkManager has _1 pending submissions...", submissions_.size()));

        for (auto it = submissions_.begin(); it != submissions_.end(); ++it)
        {
            auto& submission = *it;
            if (submission.reply)
            {
                submission.reply->blockSignals(true);
                submission.reply->abort();
            }
        }
    }

    DEBUG("NetworkManager destroyed");
}


void NetworkManager::submit(on_reply callback, Context& ctx, const QUrl& url)
{
    NetworkManager::submission get;
    get.id       = submissions_.size() + 1;
    get.ticks    = 0;
    get.reply    = nullptr;
    get.ctx      = &ctx;
    get.callback = std::move(callback);
    get.request.setUrl(url);    
    get.request.setRawHeader("User-Agent", "NewsflashPlus");    

    submissions_.push_back(get);
    ctx.num_pending_requests_++;

    if (submissions_.size() == 1)
    {
        submitNext();
        timer_.setInterval(1000);
        timer_.start();
    }

    DEBUG(str("New HTTP/GET submission _1", get.id));
}

void NetworkManager::submit(on_reply callback, Context& ctx, const QUrl& url, const Attachment& item)
{
    const auto BOUNDARY = "--abcdef123abcdef123";        

    NetworkManager::submission post;
    post.id       = submissions_.size() + 1;
    post.ticks    = 0;
    post.reply    = nullptr;
    post.ctx      = &ctx;
    post.callback = std::move(callback);
    post.attachment.append(QString("--%1\r\n").arg(BOUNDARY).toAscii());
    post.attachment.append(QString("Content-Disposition: form-data; name=\"attachment\"; filename=\"%1\"\r\n").arg(item.name).toAscii());
    post.attachment.append(QString("Content-Type: application/octet-stream\r\n\r\n").toAscii());
    post.attachment.append(item.data);
    post.attachment.append(QString("\r\n--%1--\r\n").arg(BOUNDARY).toAscii());
    post.request.setUrl(url);
    post.request.setRawHeader("User-Agent", "NewsflashPlus");
    post.request.setRawHeader("Content-Type", QString("multipart/form-data; boundary=%1").arg(BOUNDARY).toAscii());
    post.request.setRawHeader("Content-Length", QString::number(post.attachment.size()).toAscii());

    submissions_.push_back(post);

    ctx.num_pending_requests_++;

    if (submissions_.size() == 1)
    {
        submitNext();
        timer_.setInterval(1000);
        timer_.start();
    }

    DEBUG(str("New HTTP/POST submission _1", post.id));
}

void NetworkManager::cancel(Context& c)
{
    for (auto it = submissions_.begin(); it != submissions_.end(); ++it)
    {
        auto& submission = *it;
        if (submission.ctx != &c)
            continue;

        if (submission.reply)
        {
            submission.reply->blockSignals(true);
            submission.reply->abort();
        }
        DEBUG(str("Canceled submission _1", submission.id));
        it = submissions_.erase(it);        
    }
    c.num_pending_requests_ = 0;
}

void NetworkManager::finished(QNetworkReply* reply)
{
    auto it = std::find_if(std::begin(submissions_), std::end(submissions_),
        [=](const submission& s) {
            return s.reply == reply;
        });
    Q_ASSERT(it != std::end(submissions_));

    auto& submit   = *it;
    const auto err = reply->error();
    const auto url = reply->url();

    DEBUG(str("Got network reply for submission _1, _2", submit.id, str(err)));

    submit.callback(*reply);
    
    auto* ctx = submit.ctx;

    DEBUG(str("Context has _1 pending submissions", ctx->num_pending_requests_));

    if (--ctx->num_pending_requests_ == 0)
    {
        if (ctx->callback)
            ctx->callback();
    }
    submissions_.erase(it);
    reply->deleteLater();

    DEBUG(str("Pending submissions _1", submissions_.size()));
}

void NetworkManager::timeout()
{
    submitNext();
    updateTicks();

    if (submissions_.empty())
        timer_.stop();
}

void NetworkManager::submitNext()
{
    auto it = std::find_if(std::begin(submissions_), std::end(submissions_),
        [](const submission& s) {
            return s.reply == nullptr;
        });
    if (it == std::end(submissions_))
        return;

    auto& submit = *it;
    const auto& req = submit.request;
    const auto& url = submit.request.url();
    const auto& attachment = submit.attachment;
    if (attachment.isEmpty())
        submit.reply = qnam_.get(submit.request);
    else submit.reply = qnam_.post(submit.request, attachment);

    if (!submit.reply)
    {
        ERROR("Submit request failed...");
    }
    else
    {
        DEBUG(str("Submit HTTP request to _1", url));
    }
}

void NetworkManager::updateTicks()
{
    const auto timeout_ticks = 30;

    for (auto it = submissions_.begin(); it != submissions_.end(); ++it)
    {
        auto& submission = *it;
        if (!submission.reply)
            continue;

        if (++submission.ticks == timeout_ticks)
        {
            DEBUG(str("Timedout submission _1", submission.id));            
            submission.reply->blockSignals(true);
            submission.reply->abort();
            submission.callback(*submission.reply);

            auto* ctx = submission.ctx;
            if (--ctx->num_pending_requests_ == 0)
            {
                if ( ctx->callback)
                    ctx->callback();
            }

            it = submissions_.erase(it);            
        }
    }
}

const char* str(QNetworkReply::NetworkError err)
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