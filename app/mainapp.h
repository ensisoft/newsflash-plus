// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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
#  include <QObject>
#include <newsflash/warnpop.h>

#include <memory>
#include <vector>
#include <list>
#include "datastore.h"
#include "settings.h"
#include "accounts.h"
#include "groups.h"
#include "eventlog.h"
#include "groups.h"

class QAbstractItemModel;
class QCoreApplication;
class QTimerEvent;
class QEvent;
class QNetworkReply;

namespace app
{
    class mainmodel;
    class netreq;

    class mainapp : public QObject
    {
        Q_OBJECT

    public:
        mainapp();
       ~mainapp();

        // load the previously (prio application run) persisted settings 
        // and restore the application state.
        void loadstate();

        // try to take a snapshot and persist the current application state
        // so that the state can be recovered later on. 
        // returns true if succesful, otherwise false.
        bool savestate();

        // 
        void shutdown();

        void get(app::settings& settings);
        void set(const app::settings& settings);

        // submit a new network request. ownership of the
        // request remains with the model doing the submission.
        // a completion callback will be invoked on the model
        // object once the request has been completed.
        void submit(mainmodel* model, netreq* request);

        // attach a new module to the application. ownership of the module
        // remains with the caller.
        void attach(mainmodel* model);

    private slots:
        void replyAvailable(QNetworkReply* reply);

    private:
        void timerEvent(QTimerEvent* event);

    private:
        bool submit_first();

    private:
        struct submission {
            std::size_t ticks;
            netreq* request;
            mainmodel* model;
            QNetworkReply* reply;
        };        

        std::vector<mainmodel*> models_;
        std::list<submission> submits_;
        QNetworkAccessManager net_;
        int net_submit_timer_;
        int net_timeout_timer_;
    };

    extern mainapp* g_app;

} // app


