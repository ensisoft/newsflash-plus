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

#define LOGTAG "app"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <boost/version.hpp>
#  include <QtNetwork/QNetworkReply>
#  include <QtNetwork/QNetworkRequest>
#  include <QCoreApplication>
#  include <QStringList>
#  include <QFile>
#  include <QDir>
#  include <QEvent>
#  include <QLibrary>
#include <newsflash/warnpop.h>

#include <newsflash/engine/ui/account.h>
#include <newsflash/engine/engine.h>
#include <newsflash/engine/listener.h>
#include <vector>
#include "mainapp.h"
#include "mainmodel.h"
#include "eventlog.h"
#include "accounts.h"
#include "groups.h"
#include "debug.h"
#include "format.h"
#include "netreq.h"
#include "homedir.h"

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
mainapp* g_app;

mainapp::mainapp() : net_submit_timer_(0), net_timeout_timer_(0)
{
    QObject::connect(&net_, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(replyAvailable(QNetworkReply*)));

    submission_id_ = 1;

    DEBUG("Application created");        
}

mainapp::~mainapp()
{
    DEBUG("Application deleted");
}

void mainapp::loadstate()
{
    datastore data;
    const auto file = homedir::file("app.json");
    if (QFile::exists(file))
    {
        const auto error = data.load(file);
        if (error != QFile::NoError)
        {
            ERROR(str("Failed to read settings _1, _2", file, error));
            return;
        }
    }
    for (auto* model : models_)
    {
        model->load(data);
    }
}

bool mainapp::savestate()
{
    datastore data;
    for (auto* model : models_)
    {
        model->save(data);
    }

    const auto file  = homedir::file("app.json");
    const auto error = data.save(file);
    if (error != QFile::NoError)
    {
        ERROR(str("Error saving settings _1, _2", file, error));
        return false;
    }
    return true;
}

void mainapp::shutdown()
{
    DEBUG("Shutdown");
    DEBUG(str("Have _1 pending network requests", submits_.size()));

    killTimer(net_submit_timer_);
    killTimer(net_timeout_timer_);

    for (auto it = submits_.begin(); it != submits_.end(); ++it)
    {
        auto& submission = *it;
        if (!submission.reply)
        {
            delete submission.request;
        }
        else
        {
            submission.cancel = true;
            submission.reply->abort();
        }
        it = submits_.erase(it);
    }


    DEBUG("Shutdown done");
}

void mainapp::submit(mainmodel* model, std::unique_ptr<netreq> req)
{
    const auto submit_interval_millis = 1000;

    // todo: maybe hold the request in a unique_ptr in the submission
    // in the submission object.
    submits_.push_back({submission_id_, 0, req.get(), model, nullptr, false});

    req.release();

    if (submits_.size() == 1)
    {
        submit_first();
    }   
    else if (!net_submit_timer_)
    {
        net_submit_timer_ = startTimer(submit_interval_millis);
        DEBUG(str("Starting submit timer..."));
    }

    DEBUG(str("New submission _1", submission_id_));

    ++submission_id_;
}


void mainapp::cancel_all(mainmodel* model)
{
    DEBUG("cancel all");

    // again keep in mind that calling reply::abort() will invoke the replyAVailable signal
    // so that will fuck up the iteration here.

    for (auto it = submits_.begin(); it != submits_.end(); ++it)
    {
        auto& submission = *it;

        if (submission.model != model)
            continue;

        if (!submission.reply)
        {
            delete submission.request;
        }
        else
        {
            submission.cancel = true;
            submission.reply->abort();
        }
        it = submits_.erase(it);       

        DEBUG(str("Canceled submission _1", submission.id));
    }
}

void mainapp::attach(mainmodel* model)
{
    models_.push_back(model);
}

void mainapp::replyAvailable(QNetworkReply* reply)
{
    auto it = std::find_if(std::begin(submits_), std::end(submits_),
        [=](const submission& sub) {
            return sub.reply == reply;
        });
    Q_ASSERT(it != std::end(submits_));

    auto& submit   = *it;
    const auto err = reply->error();
    const auto url = reply->url();

    DEBUG(str("Got network reply for submission _1, _2", submit.id, nstr(err)));

    if (err != QNetworkReply::NoError)
    {
        ERROR(str("Network request _1 failed, _2", url, nstr(err)));
    }

    std::unique_ptr<netreq> r(submit.request);
    if (!submit.cancel)
    {
        submit.request->receive(*reply);
        submit.model->complete(std::move(r));
        submits_.erase(it);
    }
    else
    {
        DEBUG("Submit is canceled. Ignoring...");        
    }

    reply->deleteLater();

    DEBUG(str("Pending submits _1", submits_.size()));
}
void mainapp::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == net_submit_timer_)
    {
        DEBUG("Submit timer!");

        if (!submit_first())
        {
            killTimer(net_submit_timer_);
            net_submit_timer_ = 0;
            DEBUG("Killed submit timer...");
        }
    }
    else if (event->timerId() == net_timeout_timer_)
    {
        const auto timeout_ticks = 30;

        std::for_each(std::begin(submits_), std::end(submits_),
            [](submission& m) {
                if (m.reply)
                    ++m.ticks;
            });

        std::list<submission> pending;
        std::list<submission> timeouts;

        // very carefully now, calling abort() on QNetworkReply
        // will immediately invoke the finished() signal handler
        // which under normal operation removes the submission
        // from the queue. so must take care not to invalidate
        // our iteration here at any point!

        std::partition_copy(
            std::begin(submits_), std::end(submits_),
            std::back_inserter(timeouts), std::back_inserter(pending),
            [](const submission& m) {
                return m.ticks == timeout_ticks;
            });

        std::for_each(std::begin(timeouts), std::end(timeouts),
            [](submission& m) {
                m.reply->abort();
            });
        submits_ = std::move(pending);

        if (submits_.empty())
        {
            killTimer(net_timeout_timer_);
            net_timeout_timer_ = 0;
        }
    }
    else
    {
        QObject::timerEvent(event);
    }
}

bool mainapp::submit_first()
{
    auto it = std::find_if(std::begin(submits_), std::end(submits_),
        [](const submission& sub) {
            return sub.reply == nullptr;
        });
    if (it == std::end(submits_))
        return false;

    auto& submit = *it;

    QNetworkRequest request;
    request.setRawHeader("User-Agent", "NewsflashPlus");
    
    submit.request->prepare(request);
    submit.reply = net_.get(request);
    if (submit.reply == nullptr)
    {
        ERROR(str("Network get failed!"));
        //submit.handler->fa

        submits_.erase(it);
    }
    if (!net_timeout_timer_)
        net_timeout_timer_ = startTimer(1000);


    return true;
}
} // app
