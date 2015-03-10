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

#define LOGTAG "web"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtNetwork/QNetworkAccessManager>
#include <newsflash/warnpop.h>
#include "webengine.h"
#include "webquery.h"
#include "debug.h"

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

WebEngine::WebEngine()
{
    DEBUG("WebEngine created");

    QObject::connect(&qnam_, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(finished(QNetworkReply*)));
    QObject::connect(&timer_, SIGNAL(timeout()),
        this, SLOT(heartbeat()));
}

WebEngine::~WebEngine()
{
    if (!queries_.empty())
    {
        timer_.stop();
        timer_.blockSignals(true);
        qnam_.blockSignals(true);
        DEBUG("WebEngine has %1 pending queries...", queries_.size());

        for (auto& query : queries_)
            query.abort();
    }

    DEBUG("WebEngine deleted");
}

WebQuery* WebEngine::submit(WebQuery query)
{
    queries_.push_back(std::move(query));
    auto& back = queries_.back();

    timer_.setInterval(1000);
    timer_.start();
    if (queries_.size() == 1)
        heartbeat();

    return &back;
}

void WebEngine::finished(QNetworkReply* reply)
{
    auto it = std::begin(queries_);
    for (; it != std::end(queries_); ++it)
    {
        auto& query = *it;
        if (query.receive(*reply))
            break;
    }        
    ENDCHECK(queries_, it);

    queries_.erase(it);
    reply->deleteLater();
}

void WebEngine::heartbeat()
{
    // first see if there's a new query to be submitted.
    auto it = std::find_if(std::begin(queries_), std::end(queries_),
        [](const WebQuery& query) {
            return !query.isActive() && !query.isAborted();
        });
    if (it != std::end(queries_))
    {
        auto& query = *it;
        query.submit(qnam_);
    }

    // remove queries that were aborted before being submitted.
    auto end = std::remove_if(std::begin(queries_), std::end(queries_),
        [&](const WebQuery& query) {
            return query.isAborted();
        });
    queries_.erase(end, std::end(queries_));

    // important. need to block qnam signals here
    // because if we manually timeout a QNetworkRequest (and call abort)
    // it will asynchronousnly invoke the finished signal with Aborted error status.
    // this would mess up our iteration here.
    qnam_.blockSignals(true);

    // tick live queries and abort the ones that have timed out.
    for (auto it = std::begin(queries_); it != std::end(queries_); )
    {
        auto& query = *it;
        if (!query.isActive())
        {
            ++it;
            continue;
        }
        if (query.tick())
        {
            ++it;
            continue;
        }

        query.abort();
        it = queries_.erase(it);
    }

    qnam_.blockSignals(false);

    if (queries_.empty())
        timer_.stop();
}

WebEngine* g_web;

} // app