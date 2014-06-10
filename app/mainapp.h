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
#  include <QObject>
#include <newsflash/warnpop.h>

#include <newsflash/engine/listener.h>
#include <newsflash/engine/engine.h>
#include <newsflash/sdk/datastore.h>
#include <newsflash/sdk/model.h>


#include <memory>
#include <map>
#include "accounts.h"
#include "groups.h"
#include "eventlog.h"
#include "groups.h"

class QAbstractItemModel;
class QCoreApplication;
class QEvent;

namespace sdk {
    class model;
    struct msg_shutdown;
    struct msg_request_shutdown;
}

namespace app
{
    // we need a class so that we can connect Qt signals
    class mainapp : public QObject, public newsflash::listener
    {
        Q_OBJECT

    public:
        mainapp(QCoreApplication& app);
       ~mainapp();

        sdk::model& get_model(const QString& name);

        bool savestate();

        void shutdown();

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

    private:
        bool eventFilter(QObject* object, QEvent* event);

    private:


    private:
        std::unique_ptr<newsflash::engine> engine_;

    private:
        QCoreApplication& app_;    

    private:
        sdk::datastore settings_;

        // built-in models which are always available
    private:
        accounts   accounts_;
        groups     groups_;

    };

} // app
