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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QObject>
#  include <QString>
#  include <QMetaType>
#include "newsflash/warnpop.h"

#include <memory>
#include <vector>
#include <string>
#include <map>

#include "engine/ui/connection.h"
#include "engine/ui/task.h"
#include "engine/engine.h"
#include "platform.h"
#include "format.h"
#include "accounts.h"
#include "media.h"

class QEvent;

namespace app
{
    class Settings;
    struct NZBContent;
    struct Download;

    struct FileInfo;
    struct FilePackInfo;
    struct NewsGroupInfo;
    struct HeaderInfo;
    struct HeaderUpdateInfo;

    // manager class around newsflash engine + engine state
    // translate between native c++ and Qt types and events.
    class Engine : public QObject
    {
        Q_OBJECT

    public:
        Engine();
       ~Engine();

        QString resolveDownloadPath(const QString& path, MainMediaType media) const;

        void testAccount(const Accounts::Account& acc);

        // set/modify account in the engine.
        void setAccount(const Accounts::Account& acc);

        // delete an account.
        void delAccount(const Accounts::Account& acc);

        // set the current fill account.
        void setFillAccount(quint32 id);

        bool downloadNzbContents(const Download& download, const QByteArray& nzb);
        bool downloadNzbContents(const Download& download, std::vector<NZBContent> nzb);

        // retrieve a newgroup listing from the specified account.
        // returns a task id that can be used to manage the task.
        quint32 retrieveNewsgroupListing(quint32 acc);

        quint32 retrieveHeaders(quint32 acc, const QString& path, const QString& name);

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
        void refreshTaskList(std::deque<newsflash::ui::TaskDesc>& list)
        {
            engine_->GetTasks(&list);
        }

        // refresh the list of UI connection states
        void refreshConnList(std::deque<newsflash::ui::Connection>& list)
        {
            engine_->GetConns(&list);

            totalspeed_ = 0;

            // at any given moment the current engine combined download
            // speed should equal that of of all it's connections.
            // however this is the only place where we are able to get that
            // information. so in order to be able to call get_download_speed()
            // reliable a call to update_conn_list has to be made first.
            for (const auto& ui : list)
                totalspeed_ += ui.bps;
        }

        // get the free disk space on the disk where the downloadPath points to.
        quint64 getFreeDiskSpace(const QString& downloadPath) const
        {
            const auto& mountPoint = app::resolveMountPoint(downloadPath);
            const auto  freeSpace  = app::getFreeDiskSpace(mountPoint);
            return freeSpace;
        }

        // get current total download speed (of all connections)
        quint32 getDownloadSpeed() const
        {
            return totalspeed_;
        }

        // get the number of bytes downloaded from all servers/accounts.
        quint64 getBytesDownloaded() const
        {
            return engine_->GetTotalBytesDownloaded();
        }

        // get the bytes currently queued in engine for downloading.
        quint64 getBytesQueued() const
        {
            return engine_->GetCurrentQueueSize();
        }

        quint64 getBytesQueued(const QString& mountPoint);

        // get the number of bytes currently ready.
        // bytes ready is always a fraction of bytesQueued
        // so if bytesQueued is zero then bytesReady is also zero.
        quint64 getBytesReady() const
        {
            return engine_->GetBytesReady();
        }

        // get the number of bytes written to the disk
        quint64 getBytesWritten() const
        {
            return engine_->GetTotalBytesWritten();
        }

        const QString& getLogfilesPath() const
        {
            return logifiles_;
        }

        QString getEngineLogfile() const
        {
            return fromUtf8(engine_->GetLogfileName());
        }

        void setLogfilesPath(const QString& path)
        {
            logifiles_ = path;
        }

        QString getDownloadPath(MainMediaType media) const
        {
            //return downloads_;
            auto it = downloads_.find(media);
            if (it == std::end(downloads_))
                return "";
            return it->second;
        }

        // Set the default download path associated with a specific
        // type of media. When download happens if no path is given
        // then the default path associated with the media type
        // is used instead.
        void setDownloadPath(MainMediaType media, const QString& path)
        {
            downloads_[media] = path;
        }

        bool getOverwriteExistingFiles() const
        {
            return engine_->GetOverwriteExistingFiles();
        }

        bool getDiscardTextContent() const
        {
            return engine_->GetDiscardTextContent();
        }

        bool getPreferSecure() const
        {
            return engine_->GetPreferSecure();
        }

        bool isStarted() const
        {
            return engine_->IsStarted();
        }

        bool getConnect() const
        {
            return connect_;
        }

        bool getThrottle() const
        {
            return engine_->GetEnableThrottle();
        }

        unsigned getThrottleValue() const
        {
            return engine_->GetThrottleValue();
        }

        bool getCheckLowDisk() const
        { return checkLowDisk_; }

        void setCheckLowDisk(bool on_off)
        { checkLowDisk_ = on_off; }

        void setOverwriteExistingFiles(bool on_off)
        {
            engine_->SetOverwriteExistingFiles(on_off);
        }

        void setDiscardTextContent(bool on_off)
        {
            engine_->SetDiscardTextContent(on_off);
        }

        void setPreferSecure(bool on_off)
        {
            engine_->SetPreferSecure(on_off);
        }

        void setGroupSimilar(bool on_off)
        {
            engine_->SetGroupItems(on_off);
        }

        void setThrottle(bool on_off)
        {
            engine_->SetEnableThrottle(on_off);
        }

        void setThrottleValue(unsigned val)
        {
            engine_->SetThrottleValue(val);
        }

        void killConnection(std::size_t index)
        {
            engine_->KillConnection(index);
        }

        void cloneConnection(std::size_t index)
        {
            engine_->CloneConnection(index);
        }

        void killTaskById(quint32 id)
        {
            engine_->KillTaskById(id);
        }

        bool lockTaskById(quint32 id)
        {
            return engine_->LockTaskById(id);
        }

        bool unlockTaskById(quint32 id)
        {
            return engine_->UnlockTaskById(id);
        }

        void killTask(std::size_t index)
        {
            const auto id = engine_->KillTask(index);
            if (id)
            {
                emit actionKilled(id);
            }
        }

        void pauseTask(std::size_t index)
        {
            engine_->PauseTask(index);
        }

        void resumeTask(std::size_t index)
        {
            engine_->ResumeTask(index);
        }

        void moveTaskUp(std::size_t index)
        {
            engine_->MoveTaskUp(index);
        }

        void moveTaskDown(std::size_t index)
        {
            engine_->MoveTaskDown(index);
        }

    signals:
        void shutdownComplete();
        void newDownloadQueued(const Download& download);
        void newHeaderInfoAvailable(const app::HeaderUpdateInfo& info);
        void fileCompleted(const app::FileInfo& file);
        void packCompleted(const app::FilePackInfo& pack);
        void listCompleted(quint32 account, const QList<app::NewsGroupInfo>& list);
        void listUpdate(quint32 account, const app::NewsGroupInfo& group);
        void updateCompleted(const app::HeaderInfo& headers);
        void allCompleted();
        void numPendingTasks(std::size_t num);
        void quotaUpdate(std::size_t bytes, std::size_t account);
        void actionKilled(quint32 action);
        void testAccountComplete(bool success);
        void testAccountLogMsg(const QString& msg);

    private:
        virtual bool eventFilter(QObject* object, QEvent* event) override;
        virtual void timerEvent(QTimerEvent* event) override;

    private:
        void start();

    private:
        void onError(const newsflash::ui::SystemError& error);
        void onTaskComplete(const newsflash::ui::TaskDesc& task);
        void onFileComplete(const newsflash::ui::FileResult& file);
        void onBatchComplete(const newsflash::ui::FileBatchResult& batch);
        void onListComplete(const newsflash::ui::GroupListResult& list);
        void onUpdateComplete(const newsflash::ui::HeaderResult& result);
        void onHeaderInfoAvailable(const newsflash::ui::HeaderUpdate& info);
        void onListingUpdate(const newsflash::ui::GroupListUpdate& update);
        void onAllComplete();
        void onQuota(std::size_t bytes, std::size_t account);
        void onConnectionTestComplete(bool success);
        void onConnectionTestLogMsg(const std::string& msg);

    private:
        // the actual lower level engine instance.
        std::unique_ptr<newsflash::Engine> engine_;

        // where engine log files are written.
        QString logifiles_;

        // current total download speed
        quint32 totalspeed_ = 0;

        // handle to tick timer which pumps the low level engine
        int ticktimer_ = 0;

        // mapping of media types to download paths.
        // todo: this should move away from here, but ok..
        std::map<MainMediaType, QString> downloads_;

        // some flags.
        bool connect_ = true;
        bool shutdown_ = false;
        bool checkLowDisk_ = true;
    };

    extern Engine* g_engine;

} // app
