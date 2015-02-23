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
#include "accounts.h"

class QEvent;

namespace app
{
    class Settings;
    struct NZBContent;
    struct DataFileInfo;
    struct FileBatchInfo;
    struct NewsGroupInfo;

    // manager class around newsflash engine + engine state
    // translate between native c++ and Qt types and events.
    class Engine : public QObject
    {
        Q_OBJECT
        
    public:
        Engine();
       ~Engine();

        // set/modify account in the engine.
        void setAccount(const Accounts::Account& acc);

        // delete an account.
        void delAccount(const Accounts::Account& acc);

        // set the current fill account.
        void setFillAccount(quint32 id);

        bool downloadNzbContents(quint32 acc, const QString& path, const QString& desc, const QByteArray& nzb);
        bool downloadNzbContents(quint32 acc, const QString& path, const QString& desc, 
            const std::vector<const NZBContent*>& nzb);

        // retrieve a newgroup listing from the specified account.
        // returns a task id that can be used to manage the task.
        quint32 retrieveNewsgroupListing(quint32 acc);

        void loadState(Settings& s);
        void saveState(Settings& s);

        void loadSession();
        void saveSession();

        void connect(bool on_off);

        // refresh the engine state such as download speed, available disk spaces etc.
        void refresh();

        // begin engine shutdown. returns true if complete immediately
        // otherwise false in which case the a signal is emitted
        // once all pending actions have been processed in the engine.
        bool shutdown();

        // refresh the list of UI task states.
        void refreshTaskList(std::deque<newsflash::ui::task>& list)
        {
            engine_->update(list);
        }

        // refresh the list of UI connection states
        void refreshConnList(std::deque<newsflash::ui::connection>& list)
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

        // get the free disk space at the default download location.
        quint64 getFreeDiskSpace() const
        { 
            return diskspace_; 
        }

        // get current total download speed (of all connections)
        quint32 getDownloadSpeed() const
        {
            return totalspeed_;
        }

        // get the number of bytes downloaded from all servers/accounts.
        quint64 getBytesDownloaded() const
        {
            return engine_->get_bytes_downloaded();
        }

        // get the bytes currently queued in engine for downloading.
        quint64 getBytesQueued() const
        {
            return engine_->get_bytes_queued();
        }

        // get the number of bytes currently ready. 
        // bytes ready is always a fraction of bytesQueued
        // so if bytesQueued is zero then bytesReady is also zero.
        quint64 getBytesReady() const 
        {
            return engine_->get_bytes_ready();
        }

        // get the number of bytes written to the disk 
        quint64 getBytesWritten() const 
        {
            return engine_->get_bytes_written();
        }

        const QString& getLogfilesPath() const
        { 
            return logifiles_; 
        }

        QString getEngineLogfile() const
        {
            return fromUtf8(engine_->get_logfile());
        }

        void setLogfilesPath(const QString& path)
        {
            logifiles_ = path;
        }

        const QString& getDownloadPath() const 
        { 
            return downloads_; 
        }

        void setDownloadPath(const QString& path)
        {
            downloads_  = path;
            mountpoint_ = resolveMountPoint(downloads_);
        }

        bool getOverwriteExistingFiles() const
        {
            return engine_->get_overwrite_existing_files();
        }

        bool getDiscardTextContent() const 
        { 
            return engine_->get_discard_text_content();
        }

        bool getPreferSecure() const
        {
            return engine_->get_prefer_secure();
        }

        bool isStarted() const
        {
            return engine_->is_started(); 
        }

        bool getConnect() const
        { 
            return connect_; 
        }

        bool getThrottle() const 
        {
            return engine_->get_throttle();
        }

        void setOverwriteExistingFiles(bool on_off)
        {
            engine_->set_overwrite_existing_files(on_off);
        }

        void setDiscardTextContent(bool on_off)
        {
            engine_->set_discard_text_content(on_off);
        }

        void setPreferSecure(bool on_off)
        {
            engine_->set_prefer_secure(on_off);
        }

        void setGroupSimilar(bool on_off)
        {
            engine_->set_group_items(on_off);
        }

        void setThrottle(bool on_off)
        {
            engine_->set_throttle(on_off);
        }

        void setThrottleValue(unsigned val)
        {
            engine_->set_throttle_value(val);
        }

        void killConnection(std::size_t index)
        {
            engine_->kill_connection(index);
        }

        void cloneConnection(std::size_t index)
        {
            engine_->clone_connection(index);
        }

        void killTask(std::size_t index)
        {
            engine_->kill_task(index);
        }

        void pauseTask(std::size_t index)
        {
            engine_->pause_task(index);
        }

        void resumeTask(std::size_t index)
        {
            engine_->resume_task(index);
        }

        void moveTaskUp(std::size_t index)
        {
            engine_->move_task_up(index);
        }

        void moveTaskDown(std::size_t index)
        {
            engine_->move_task_down(index);
        }

    signals:
        void shutdownComplete();
        void newDownloadQueued(const QString& desc);
        void fileCompleted(const app::DataFileInfo& file);
        void batchCompleted(const app::FileBatchInfo& batch);
        void listingCompleted(quint32 account, const QList<app::NewsGroupInfo>& list);

    private:
        virtual bool eventFilter(QObject* object, QEvent* event) override;
        virtual void timerEvent(QTimerEvent* event) override;

    private:
        void start();

    private:
        void onError(const newsflash::ui::error& e);
        void onFileComplete(const newsflash::ui::file& f);
        void onBatchComplete(const newsflash::ui::batch& b);
        void onListComplete(const newsflash::ui::listing& l);

    private:
        QString logifiles_;
        QString downloads_;
        QString mountpoint_;
        quint64 diskspace_;
        quint32 totalspeed_;
        int ticktimer_;
        std::unique_ptr<newsflash::engine> engine_;
        bool connect_;
        bool shutdown_;
    };

    extern Engine* g_engine;

} // app
