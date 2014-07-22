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
#  include <QString>
#include <newsflash/warnpop.h>
#include <memory>
#include <string>
#include <deque>

namespace newsflash {
    class engine;
    class settings;
}

class QCoreApplication;
class QEvent;

namespace app
{
    class account;

    // manager class around newsflash engine + engine state
    class engine : public QObject
    {
        Q_OBJECT
    public:
        struct file {
            QString description;

        };

        engine(QCoreApplication& application);
       ~engine();

        void set(const account& acc);

        //void download()
    private:
        virtual bool eventFilter(QObject* object, QEvent* event) override;

    private:
        class listener;
    private:
        QCoreApplication& app_;
    private:
        // std::unique_ptr<newsflash::engine> engine_;
        // std::unique_ptr<newsflash::settings> settings_;
        // std::unique_ptr<listener> listener_;
    private:

    };

    extern engine* g_engine;

} // app
