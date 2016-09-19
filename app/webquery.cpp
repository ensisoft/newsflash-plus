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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtNetwork/QNetworkAccessManager>
#include "newsflash/warnpop.h"

#include "webquery.h"
#include "debug.h"

#define S(x) QString(x)

namespace app
{

WebQuery::~WebQuery()
{
    if (m_reply)
        m_reply->deleteLater();
}

void append(QByteArray& buff,  QString str)
{
    buff.append(str.toAscii());
}

void append(QByteArray& buff, const QByteArray& in)
{
    buff.append(in);
}

bool WebQuery::submit(QNetworkAccessManager& qnam)
{
    Q_ASSERT(m_reply == nullptr && "Query has already been submitted.");
    if (m_aborted)
        return false;

    QNetworkRequest request;
    request.setRawHeader("User-Agent", "NewsflashPlus");
    request.setUrl(m_url);

    if (!haveAttachment())
    {
        m_reply = qnam.get(request);
    }
    else
    {
        const auto BOUNDARY = "--abcdef123abcdef123";            
        QByteArray buff;
        append(buff, S("--%1\r\n").arg(BOUNDARY));
        append(buff, S("Content-Disposition: form-data; name=\"attachment\"; filename=\"%1\"\r\n").arg(m_attchName));
        append(buff, S("Content-Type: application/octect-stream\r\n\r\n"));
        append(buff, m_attchData);
        append(buff, S("\r\n--%1--\r\n").arg(BOUNDARY));
        request.setRawHeader("Content-Type", S("multipart/form-data; boundary=%1").arg(BOUNDARY).toAscii());
        request.setRawHeader("Content-Length", QString::number(buff.size()).toAscii());
        m_reply = qnam.post(request, buff);
    }
    DEBUG("WebQuery to %1 submitted...", m_url);
    return true;
}

bool WebQuery::tick(unsigned maxTicks)
{
    if (++m_ticks < maxTicks)
        return true;

    return false;
}

bool WebQuery::receive(QNetworkReply& reply)
{
    if (&reply != m_reply)
        return false;

    // if the user has aborted the query, but the query has already
    // finished at time but the event has not been processed,
    // we simply discard the reply.
    if (m_aborted)
        return true;

    DEBUG("WebQuery to %1 completed with %2", m_url, reply.error());

    OnReply(reply);
    return true;
}

bool WebQuery::isActive() const 
{
    return m_reply != nullptr;
}

bool WebQuery::isAborted() const 
{
    return m_aborted;
}

bool WebQuery::isTimeout() const 
{
    return m_timeout;
}

void WebQuery::abort()
{
    m_aborted = true;
    if (!m_reply)
        return;

    // aborting the query is done by the client of WebQuery.
    // which means that they're no longer interested in the completion
    // of the query (so the completion callback will not be fired).
    // Hence we must block the signal here before calling
    // abort on the reply, otherwise the signal will fire.

    m_reply->blockSignals(true);
    m_reply->abort();
    DEBUG("WebQuery to %1 aborted", m_url);
}

void WebQuery::timeout()
{
    m_timeout = true;

    DEBUG("WebQuery to %1 timed out", m_url);

    // NOTE: calling abort() will invoke the signal for completion.
    // and for timeout() this is what we want, basically this means that 
    // the query completed but *with* errors.
    m_reply->abort();
}

bool WebQuery::haveAttachment() const 
{
    return !m_attchName.isEmpty() &&
           !m_attchData.isEmpty();
}

} //app
