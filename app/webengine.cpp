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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtNetwork/QNetworkAccessManager>
#  include <QThread>
#include "newsflash/warnpop.h"

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

// site for testing.
// http://httpbin.org/

namespace {
    constexpr auto kNumTicksPerSecond  = 2;
    constexpr auto kTickIntervalMillis = 1000 / kNumTicksPerSecond;
}

namespace app
{

WebEngine::WebEngine()
{
    DEBUG("WebEngine created");

    QObject::connect(&m_qnam, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(finished(QNetworkReply*)));
    QObject::connect(&m_timer, SIGNAL(timeout()),
        this, SLOT(heartbeat()));

    m_timer.setInterval(kTickIntervalMillis);
    m_timer.start();

    setTimeout(DefaultTimeout);
}

WebEngine::~WebEngine()
{
    m_timer.stop();
    m_timer.blockSignals(true);
    m_qnam.blockSignals(true);
    DEBUG("WebEngine has %1 pending queries...", m_active.size());

    for (auto& query : m_active)
    {
        if (query.reply)
            query.reply->deleteLater();
    }

    DEBUG("WebEngine deleted");
}

WebQuery* WebEngine::submit(WebQuery query)
{
    auto q = std::make_shared<WebQuery>(std::move(query));

    WebQuery* ret = q.get();

    QueryState state;
    state.reply = q->Submit(m_qnam);
    state.query = q;
    // put in the set of active currently running queries.
    m_active.push_back(state);

    // set a flag that is used to limit the allcomplete signal
    // to signal edge only.
    m_signalled = false;

    return ret;
}

void WebEngine::setTimeout(std::size_t seconds)
{
    // convert timeout seconds to ticks
    m_timeoutTicks = kNumTicksPerSecond * seconds;
}

void WebEngine::finished(QNetworkReply* reply)
{
    DEBUG("Finished reply handler!");
    DEBUG("Current thread %1", QThread::currentThreadId());

    for (auto it = m_active.begin(); it != m_active.end(); ++it)
    {
        QueryState& query  = *it;
        if (query.reply != reply)
            continue;

        // if we get a signal for a query that has non-active state
        // (for example aborted) we simply ignore the signal
        if (query.query->GetState() == WebQuery::State::Active)
        {
            // invoke completion
            query.query->Complete(*query.reply);
        }
        m_active.erase(it);
        break;
    }
    reply->blockSignals(true);
    reply->deleteLater();
}

void WebEngine::heartbeat()
{
    // handle/tick currently active queries.
    for (auto it = m_active.begin(); it != m_active.end();)
    {
        auto& query = *it;

        // tick and optinally timeout.
        if (query.query->GetState() == WebQuery::State::Active &&
           !query.query->tick(m_timeoutTicks))
        {
            ASSERT(query.reply);
            query.reply->blockSignals(true);
            // we call abort here and then invoke the timeout callback
            // this will set the reply state to CanceledError which is
            // the behaviour from before (Qt4 time QNAM)
            query.reply->abort();
            query.reply->deleteLater();
            query.query->Timeout(*query.reply);
            query.reply = nullptr;
            // delete
            it = m_active.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // handle aborted queries
    for (auto it = m_active.begin(); it != m_active.end(); )
    {
        auto& query = *it;
        if (query.query->GetState() == WebQuery::State::Aborted)
        {
              // if there's already a pending QNetworkReply then abort it.
            if (query.reply)
            {
                query.reply->blockSignals(true);
                query.reply->abort();
                query.reply->deleteLater();
                query.reply = nullptr;
            }
            // delete
            it = m_active.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (m_active.empty() && !m_signalled)
    {
        emit allFinished();
        m_signalled = true;
    }
}

WebEngine* g_web;

} // app
