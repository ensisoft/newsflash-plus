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

#pragma once

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkAccessManager>
#  include <QtNetwork/QNetworkReply>
#  include <QObject>
#  include <QString>
#  include <QUrl>
#  include <QTimer>
#include <newsflash/warnpop.h>
#include <memory>
#include <list>
#include <functional>

class QNetworkReply;
class QNetworkRequest;

namespace app
{
    class netman;

    // simple network manager class
    class netman : public QObject
    {
        Q_OBJECT

    public:
        class context
        {
        public:
            // callback to be invoked (per token) when all requests
            // within that token are ready.
            using on_ready = std::function<void()>;

            on_ready callback;

            context() : num_pending_requests_(0)
            {}
            context(netman& m) : num_pending_requests_(0), manager_(&m)
            {}

           ~context()
            {
                manager_->cancel(*this);
            }

        private:
            friend class netman;
        private:
            std::size_t num_pending_requests_;
        private:
            netman* manager_;
        };

        // callback to be invoked when a request has completed.
        using on_reply = std::function<void(QNetworkReply&)>;

        netman();
       ~netman();

        context get_submission_context()
        { return context(*this); }

        // submit a new request to retrieve the content
        // at the specified URL.
        void submit(on_reply callback, context& ctx, const QUrl& url);

        struct attachment {
            QString name;
            QByteArray data;
        };

        void submit(on_reply callback, context& ctx, const QUrl& url,
            const attachment& item);

        // cancel pending submissions. callback will not be invoked.
        void cancel(context& c);

    private slots:
        void finished(QNetworkReply* reply);
        void timeout();

    private:
        void submit_next();
        void update_ticks();

    private:
        // network submission
        struct submission {
            std::size_t id;
            std::size_t ticks;
            QNetworkReply* reply;
            QNetworkRequest request;
            QByteArray attachment;
            context* ctx;            
            on_reply callback;
        };
    private:
        std::list<submission> submissions_;
        QNetworkAccessManager qnam_;
        QTimer timer_;
    };

    const char* str(QNetworkReply::NetworkError err);

    extern netman* g_net;

} // app