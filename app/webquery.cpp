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

#include "engine/assert.h"
#include "webquery.h"
#include "debug.h"

#define S(x) QString(x)

namespace app
{

void append(QByteArray& buff,  QString str)
{
    buff.append(str.toAscii());
}

void append(QByteArray& buff, const QByteArray& in)
{
    buff.append(in);
}

QNetworkReply* WebQuery::Submit(QNetworkAccessManager& qnam)
{
    // should only ever be submitted when in waiting state.
    ASSERT(mState == State::Waiting);

    QNetworkRequest request;
    request.setRawHeader("User-Agent", "NewsflashPlus");
    request.setUrl(m_url);

    QNetworkReply* reply = nullptr;

    if (!haveAttachment())
    {
        reply = qnam.get(request);
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
        reply = qnam.post(request, buff);
    }
    DEBUG("WebQuery to %1 submitted...", m_url);

    // we throw this away to save space.
    m_attchName.clear();
    m_attchData.clear();

    // return the reply to the caller
    mState = State::Active;

    // todo: get rid of this
    m_reply = reply;

    return reply;
}

bool WebQuery::tick(unsigned maxTicks)
{
    if (++m_ticks < maxTicks)
        return true;

    return false;
}

void WebQuery::Complete(QNetworkReply& reply)
{
    ASSERT(mState == State::Active);

    DEBUG("WebQuery to %1 completed with %2", m_url, reply.error());

    // dangerous callback be here. todo. refactor to event/message based.
    OnReply(reply);

    mState = State::Complete;
}

bool WebQuery::isActive() const
{
    return mState == State::Active;
}

bool WebQuery::isAborted() const
{
    return mState == State::Aborted;
}

bool WebQuery::isTimeout() const
{
    return mState == State::Timedout;
}

void WebQuery::abort()
{
    // aborting the query is done by the client of WebQuery.
    // which means that they're no longer interested in the completion
    mState = State::Aborted;

    DEBUG("WebQuery to %1 aborted", m_url);
}

void WebQuery::Timeout(QNetworkReply& reply)
{
    mState = State::Timedout;

    OnReply(reply);

    DEBUG("WebQuery to %1 timed out", m_url);
}

bool WebQuery::haveAttachment() const
{
    return !m_attchName.isEmpty() &&
           !m_attchData.isEmpty();
}

} //app
