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

#include <newsflash/engine/listener.h>
#include <newsflash/engine/engine.h>

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

    // we need a class so that we can connect Qt signals
    // mainapp manages the downloading engine, implements the
    // engine callback interface, and manages the dynamic model plugins
    // providing them with hostapp implementation.
    class mainapp : public QObject, public newsflash::listener
    {
        Q_OBJECT

    public:
        mainapp(QCoreApplication& app);
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

        // todo: 
        virtual void handle(const newsflash::error& error) override;

        // todo:
        virtual void acknowledge(const newsflash::file& file) override;

        // this callback gets invoked by the engine when
        // there are new events to be processed inside
        // the engine. this is an async callback and
        // can occur from different threads.
        // async_notify implementation should arrange
        // for the "main" thread that drives the engine
        // to call engine::pump to process pending events.
        virtual void async_notify() override;

    private slots:
        void replyAvailable(QNetworkReply* reply);

    private:
        bool eventFilter(QObject* object, QEvent* event);
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
        std::unique_ptr<newsflash::engine> engine_;        
        QCoreApplication& app_;    
        QNetworkAccessManager net_;
        int net_submit_timer_;
        int net_timeout_timer_;
        settings settings_;
    };

} // app
