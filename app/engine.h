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
#  include <QMetaType>
#include <newsflash/warnpop.h>
#include <newsflash/engine/ui/connection.h>
#include <newsflash/engine/ui/task.h>
#include <newsflash/engine/engine.h>
#include <memory>
#include <vector>
#include <string>
#include "platform.h"
#include "format.h"

class QEvent;

namespace app
{
    class account;
    class settings;
    class nzbcontent;

    // file message notifies of a downloaded file that
    // is now available in the filesystem
    struct file {
        // path to the file
        QString path;

        // name of the file at the path
        QString name;

        // size of the file
        quint64 size;

        // true if apparently damaged.
        bool damaged;

        // true if data is binary.
        bool binary;
    };

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

        bool download_nzb_contents(quint32 acc, const QString& path, const QString& desc, const QByteArray& nzb);
        bool download_nzb_contents(quint32 acc, const QString& path, const QString& desc, 
            const std::vector<const nzbcontent*>& nzb);

        void loadstate(settings& s);
        bool savestate(settings& s);

        void connect(bool on_off);

        void refresh();

        // begin engine shutdown. returns true if complete immediately
        // otherwise false in which case the a signal is emitted
        // once all pending actions have been processed in the engine.
        bool shutdown();

        void update_task_list(std::deque<newsflash::ui::task>& list)
        {
            engine_->update(list);
        }

        void update_conn_list(std::deque<newsflash::ui::connection>& list)
        {
            engine_->update(list);

            totalspeed_ = 0;

            // at any given moment the current engine combined download
            // speed should equal that of of all it's connections. 
            // however this is the only place where we are able to get that
            // information. so in order to be able to call get_download_speed() 
            // reliable a call to update_conn_list has to be made first.
            for (const auto& ui : list)
                totalspeed_ += ui.bps;
        }

        quint64 get_free_disk_space() const
        { 
            return diskspace_; 
        }

        quint32 get_download_speed() const
        {
            return totalspeed_;
        }

        quint64 get_bytes_downloaded() const
        {
            return engine_->get_bytes_downloaded();
        }

        quint64 get_bytes_queued() const
        {
            return engine_->get_bytes_queued();
        }

        quint64 get_bytes_ready() const 
        {
            return engine_->get_bytes_ready();
        }

        quint64 get_bytes_written() const 
        {
            return engine_->get_bytes_written();
        }

        const QString& get_logfiles_path() const
        { 
            return logifiles_; 
        }

        QString get_engine_logfile() const
        {
            return from_utf8(engine_->get_logfile());
        }

        void set_logfiles_path(const QString& path)
        {
            logifiles_ = path;
        }

        const QString& get_download_path() const 
        { 
            return downloads_; 
        }

        void set_downloads_path(const QString& path)
        {
            downloads_  = path;
            mountpoint_ = resolve_mount_point(downloads_);
        }

        bool get_overwrite_existing_files() const
        {
            return engine_->get_overwrite_existing_files();
        }

        bool get_discard_text_content() const 
        { 
            return engine_->get_discard_text_content();
        }

        bool get_prefer_secure() const
        {
            return engine_->get_prefer_secure();
        }

        bool is_started() const
        {
            return engine_->is_started(); 
        }

        bool get_connect() const
        { 
            return connect_; 
        }

        bool get_throttle() const 
        {
            return engine_->get_throttle();
        }

        void set_overwrite_existing_files(bool on_off)
        {
            engine_->set_overwrite_existing_files(on_off);
        }

        void set_discard_text_content(bool on_off)
        {
            engine_->set_discard_text_content(on_off);
        }

        void set_prefer_secure(bool on_off)
        {
            engine_->set_prefer_secure(on_off);
        }

        void set_group_similar(bool on_off)
        {
            engine_->set_group_items(on_off);
        }

        void set_throttle(bool on_off)
        {
            engine_->set_throttle(on_off);
        }

        void set_throttle_value(unsigned val)
        {
            engine_->set_throttle_value(val);
        }

        void kill_connection(std::size_t index)
        {
            engine_->kill_connection(index);
        }

        void clone_connection(std::size_t index)
        {
            engine_->clone_connection(index);
        }

        void kill_task(std::size_t index)
        {
            engine_->kill_task(index);
        }

        void pause_task(std::size_t index)
        {
            engine_->pause_task(index);
        }

        void resume_task(std::size_t index)
        {
            engine_->resume_task(index);
        }

        void move_task_up(std::size_t index)
        {
            engine_->move_task_up(index);
        }

        void move_task_down(std::size_t index)
        {
            engine_->move_task_down(index);
        }

    signals:
        void shutdownComplete();
        void newDownloadQueued(const QString& desc);
        void fileCompleted(const app::file& file);

    private:
        virtual bool eventFilter(QObject* object, QEvent* event) override;
        virtual void timerEvent(QTimerEvent* event) override;

    private:
        void on_error(const newsflash::ui::error& e);
        void on_file_complete(const newsflash::ui::file& f);
        void on_batch_complete(const newsflash::ui::batch& b);
    private:
        QString logifiles_;
        QString downloads_;
        QString mountpoint_;
        quint64 diskspace_;
        quint32 totalspeed_;
        int ticktimer_;
    private:
        std::unique_ptr<newsflash::engine> engine_;
    private:
        bool connect_;
        bool shutdown_;
    };

    extern engine* g_engine;

} // app

    Q_DECLARE_METATYPE(app::file);
