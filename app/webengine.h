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
#  include <QtNetwork/QNetworkAccessManager>
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QObject>
#  include <QString>
#  include <QTimer>
#  include <QUrl>
#include <newsflash/warnpop.h>
#include <functional>
#include <memory>
#include <list>

namespace app
{
    class WebQuery;

    // WebEngine provides a simple interface to perform WebQueries.
    class WebEngine : public QObject
    {
        Q_OBJECT

    public:
        WebEngine();
       ~WebEngine();

        // Submit a new query. Returns a pointer to the query
        // object stored in the engine. the pointer will be valid
        // untill the query completes.
        WebQuery* submit(WebQuery query);

    private slots:
        void finished(QNetworkReply* reply);
        void heartbeat();

    private:
        QNetworkAccessManager m_qnam;
        QTimer m_timer;
        std::list<std::unique_ptr<WebQuery>> m_queries;
    };

    extern WebEngine* g_web;

} // app