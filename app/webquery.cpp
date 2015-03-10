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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtNetwork/QNetworkAccessManager>
#include <newsflash/warnpop.h>
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

bool WebQuery::submit(QNetworkAccessManager& qnam)
{
    Q_ASSERT(reply_ == nullptr && "Query has already been submitted.");
    if (abort_)
        return false;

    QNetworkRequest request;
    request.setRawHeader("User-Agent", "NewsflashPlus");
    request.setUrl(url_);

    if (!haveAttachment())
    {
        reply_ = qnam.get(request);
    }
    else
    {
        const auto BOUNDARY = "--abcdef123abcdef123";            
        QByteArray buff;
        append(buff, S("--%1\r\n").arg(BOUNDARY));
        append(buff, S("Content-Disposition: form-data; name=\"attachment\"; filename=\"%1\"\r\n").arg(attchName_));
        append(buff, S("Content-Type: application/octect-stream\r\n\r\n"));
        append(buff, attchData_);
        append(buff, S("\r\n--%1--\r\n").arg(BOUNDARY));
        request.setRawHeader("Content-Type", S("multipart/form-data; boundary=%1").arg(BOUNDARY).toAscii());
        request.setRawHeader("Content-Length", QString::number(buff.size()).toAscii());
        reply_ = qnam.post(request, buff);
    }
    DEBUG("WebQuery to %1 submitted...", url_);
    return true;
}

bool WebQuery::tick()
{
    if (++ticks_ < 30)
        return true;

    return false;
}

bool WebQuery::receive(QNetworkReply& reply)
{
    if (&reply != reply_)
        return false;

    if (abort_)
        return true;

    DEBUG("WebQuery to %1 completed with %2", url_, reply.error());

    OnReply(reply);
    return true;
}

bool WebQuery::isActive() const 
{
    return reply_ != nullptr;
}

bool WebQuery::isAborted() const 
{
    return abort_;
}

void WebQuery::abort()
{
    abort_ = true;
    if (!reply_)
        return;

    reply_->blockSignals(true);
    reply_->abort();
    DEBUG("WebQuery to %1 aborted", url_);
}

bool WebQuery::haveAttachment() const 
{
    return !attchName_.isEmpty() &&
           !attchData_.isEmpty();
}

} //app
