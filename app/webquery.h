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

#pragma once

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QByteArray>
#  include <QString>
#  include <QUrl>
#include <newsflash/warnpop.h>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

namespace app
{
    // WebQuery represents a HTTP Get/Post request.
    class WebQuery 
    {
    public:
        using OnReplyCallback = std::function<void (QNetworkReply&)>;

        // Callback to be invoked when the query has completed
        // either succesfully or with an error.
        OnReplyCallback OnReply;

        // construct a new simple query.
        WebQuery(QUrl url) : url_(url), reply_(nullptr), abort_(false), ticks_(0)
        {}

        // construct a query with attachment.
        WebQuery(QUrl url, QString attchName, QByteArray attchData) : WebQuery(url)
        {
            attchData_ = attchData;
            attchName_ = attchName;
        }

        // submit the query through QNetworkAccessManager.
        bool submit(QNetworkAccessManager& qnam);

        // tick the query. when maximum ticks are reached the
        // query is expired and timed out.
        bool tick();

        // inspect a reply. if the reply is for this query
        // invokes the OnReply callback and returns true.
        // Otherwise returns false.
        bool receive(QNetworkReply& reply);

        // returns true if the query is active. otherwise false.
        bool isActive() const;

        bool isAborted() const;

        // abort the query.
        void abort();

        bool isOwner(const QNetworkReply& reply) const
        { return &reply == reply_; }
    private:
        bool haveAttachment() const;
    private:
        QUrl url_;
        QString attchName_;
        QByteArray attchData_;
        QNetworkReply* reply_;
    private:
        bool abort_;
        unsigned ticks_;
    };

} // app