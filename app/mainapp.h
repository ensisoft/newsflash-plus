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
#include <memory>

#include "accounts.h"
#include "groups.h"

class QAbstractItemModel;

namespace app
{
    // we need a class so that we can connect Qt signals
    class mainapp : public QObject, public newsflash::listener
    {
        Q_OBJECT

    public:
        mainapp();
       ~mainapp();

        QAbstractItemModel* get_accounts_data();
        QAbstractItemModel* get_event_data();
        QAbstractItemModel* get_groups_data();
       
        virtual void handle(const newsflash::error& error) override;
        virtual void acknowledge(const newsflash::file& file) override;
        virtual void notify() override;

    private:
        std::unique_ptr<newsflash::engine> engine_;

    private:
        accounts accounts_;
        groups   groups_;

    };

} // app
