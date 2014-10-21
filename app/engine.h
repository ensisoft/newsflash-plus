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
#include <newsflash/engine/engine.h>
#include <newsflash/engine/settings.h>
#include <memory>
#include <vector>
#include <string>

class QEvent;

namespace newsflash {
    class engine;
    class settings;
} // 

namespace app
{
    class account;

    // manager class around newsflash engine + engine state
    // translate between native c++ and Qt types and events.
    class engine : public QObject
    {
        Q_OBJECT
    public:
        engine();
       ~engine();

        void set(const account& acc);
        void del(const account& acc);

        void set_fill_account(quint32 id);

        void download_nzb_contents(quint32 acc, const QString& path, const QString& desc, const QByteArray& nzb);

        // set the default download path that is used when 
        // no specific path is specified for the download.
        // void set_download_path(const QString& path)
        // { downloads_ = path; }

        // void set_logfiles_path(const QString& path)
        // { logifiles_ = path; }

        //void refresh();

    private:
        virtual bool eventFilter(QObject* object, QEvent* event) override;

    private:
        //void on_engine_error(const newsflash::ui::error& e);
        //void on_engine_file(const newsflash::ui::file& f);
    private:
        newsflash::engine engine_;
        newsflash::settings settings_;
    };

    extern engine* g_engine;

} // app
