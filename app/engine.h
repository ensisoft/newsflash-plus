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
#include <newsflash/engine/bitflag.h>
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
    class settings;

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

        void loadstate(settings& s);
        bool savestate(settings& s);

        void start();

        void stop();

        void apply_settings();

        void refresh();

        const QString& get_logfiles_path() const
        { 
            return logifiles_; 
        }

        const QString& get_download_path() const 
        { 
            return downloads_; 
        }

        bool get_overwrite_existing_files() const
        {
            return flags_.test(newsflash::engine::flags::overwrite_existing_files);
        }

        bool get_discard_text_content() const 
        { 
            return flags_.test(newsflash::engine::flags::discard_text_content);
        }

        bool get_auto_remove_complete() const
        { 
            return flags_.test(newsflash::engine::flags::auto_remove_complete);
        }

        bool get_prefer_secure() const
        {
            return flags_.test(newsflash::engine::flags::prefer_secure);
        }

        bool is_started() const
        {
            return engine_.is_started(); 
        }

        void set_overwrite_existing_files(bool on_off)
        {
            flags_.set(newsflash::engine::flags::overwrite_existing_files, on_off);
        }

        void set_discard_text_content(bool on_off)
        {
            flags_.set(newsflash::engine::flags::discard_text_content, on_off);
        }

        void set_auto_remove_complete(bool on_off)
        {
            flags_.set(newsflash::engine::flags::auto_remove_complete, on_off);
        }

        void set_download_path(const QString& path)
        {
            downloads_ = path;
        }        

        void set_logfiles_path(const QString& path)
        {
            logifiles_ = path;
        }

        void set_throttle(bool on_off, unsigned value)
        {

        }
    private:
        virtual bool eventFilter(QObject* object, QEvent* event) override;

    private:
        void on_engine_error(const newsflash::ui::error& e);
        void on_engine_file(const newsflash::ui::file& f);
    private:
        QString logifiles_;
        QString downloads_;
        quint64 diskspace_;
    private:
        newsflash::engine engine_;
        newsflash::bitflag<newsflash::engine::flags> flags_;
        bool throttle_;
        unsigned throttle_value_;
    };

    extern engine* g_engine;

} // app
