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
    // simple network manager class
    class netman : public QObject
    {
        Q_OBJECT

    public:
        // callback invoked when all actions are ready.
        std::function<void (void)> on_ready;

        using callback = std::function<void(QNetworkReply&)>;

        netman();
       ~netman();

        // submit a new request to retrieve the content
        // at the specified URL.
        void submit(callback completion, QUrl url);

        // cancel pending submissions. callback will not be invoked.
        void cancel();

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
            QUrl url;
            bool cancel;
            bool timeout;
            callback cb;
        };
    private:
        std::size_t submission_id_;
        std::list<submission> submissions_;
        QNetworkAccessManager qnam_;
        QTimer timer_;
    };

    const char* str(QNetworkReply::NetworkError err);

} // app