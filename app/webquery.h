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
        enum class State {
            Waiting,
            Active,
            Aborted,
            Complete,
            Timedout
        };

        using OnReplyCallback = std::function<void (QNetworkReply&)>;

        // Callback to be invoked when the query has completed
        // either succesfully or with an error.
        OnReplyCallback OnReply;

        // construct a new simple query.
        WebQuery(QUrl url) : m_url(url)
        {}

        // construct a query with attachment.
        WebQuery(QUrl url, QString attchName, QByteArray attchData) : WebQuery(url)
        {
            m_attchData = attchData;
            m_attchName = attchName;
        }

        State GetState() const
        { return mState; }

        // Submit the query through QNetworkAccessManager.
        // Returns the QNetworkReply* that is the reply handle.
        QNetworkReply* Submit(QNetworkAccessManager& qnam);

        // tick the query. if the current tick count exceeds the max tick count
        // returns false, otherwise true.
        bool tick(unsigned maxTicks);

        // Complete the query by inspecting the given reply and
        // doing any processing required.
        void Complete(QNetworkReply& reply);

        // returns true if the query is active. otherwise false.
        bool isActive() const;

        bool isAborted() const;

        bool isTimeout() const;

        // abort the query. this will not invoke the OnReply callback.
        void abort();

        // times out the query. this is used by the WebEngine and
        // is not intended to be called by the clients.
        void Timeout(QNetworkReply& reply);

        // fix this shit. the WebQuery should no longer hold on
        // to the QNetworkReply
        bool isOwner(const QNetworkReply* reply) const
        { return m_reply == reply; }
        bool isOwner(const QNetworkReply& reply) const
        { return m_reply == &reply; }

    private:
        bool haveAttachment() const;
    private:
        QUrl m_url;
        QString m_attchName;
        QByteArray m_attchData;
        // todo: remove this.
        QNetworkReply* m_reply = nullptr;
    private:
        // current query state.
        State mState = State::Waiting;
        // current tick counter how long the query has been "ticking"
        unsigned m_ticks = 0;
    };

} // app