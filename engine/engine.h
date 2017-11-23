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

#include <newsflash/config.h>

#include <functional>
#include <memory>
#include <deque>
#include <vector>
#include "ui/account.h"
#include "ui/task.h"
#include "ui/connection.h"
#include "ui/download.h"
#include "ui/error.h"
#include "ui/result.h"
#include "ui/update.h"

namespace data {
    class TaskState;
}

namespace newsflash
{
    class Connection;
    class Task;
    class Logger;

    class Engine
    {
    public:
        class Factory
        {
        public:
            virtual ~Factory() = default;
            virtual std::unique_ptr<Task> AllocateTask(const ui::FileDownload& file) = 0;
            virtual std::unique_ptr<Task> AllocateTask(const ui::HeaderDownload& download) = 0;
            virtual std::unique_ptr<Task> AllocateTask(const ui::GroupListDownload& list) = 0;
            virtual std::unique_ptr<Task> AllocateTask(const data::TaskState& data) = 0;
            virtual std::unique_ptr<Connection> AllocateConnection(const ui::Account& acc) = 0;
            virtual std::unique_ptr<ui::Result> MakeResult(const Task& task, const ui::TaskDesc& desc) const = 0;
            virtual std::unique_ptr<Logger> AllocateEngineLogger() = 0;
            virtual std::unique_ptr<Logger> AllocateConnectionLogger() = 0;
        private:
        };

        // this callback is invoked when an error has occurred.
        // the error object carries information and details about
        // what happened.
        using on_error = std::function<void (const ui::SystemError& error)>;

        // this callback is invoked when a new file has been completed.
        using on_file = std::function<void (const ui::FileResult& file)>;

        // this callback is invoked when a batch is complete
        using on_batch = std::function<void (const ui::FileBatchResult& batch)>;

        // this callback is invoked when a listing is complete.
        using on_list  = std::function<void (const ui::GroupListResult& listing)>;

        // this callback is invoked when a update is complete
        using on_update = std::function<void(const ui::HeaderResult& update)>;

        // this callback is invoked when a task is complete.
        using on_task = std::function<void(const ui::TaskDesc& task)>;

        // this callback is invoked when there's new information regarding
        // an ongoing header update task.
        using on_header_update = std::function<void(const ui::HeaderUpdate& update)>;


        // this callback is invoked when there are pending events inside the engine
        // the handler function should organize for a call into engine::pump()
        // to process the pending events.
        // note that this callback handler needs to be thread safe and the notifications
        // can come from multiple threads inside the engine.
        using on_async_notify = std::function<void ()>;

        // this callback is called when all tasks are done.
        using on_finish = std::function<void()>;

        // this callback is called when payload data that affects
        // quota is downloaded.
        using on_quota = std::function<void (std::size_t bytes, std::size_t account)>;

        using on_conn_test = std::function<void (bool success)>;

        using on_conn_test_log  = std::function<void (const std::string& msgline)>;

        using TaskId = std::size_t;

        // create the engine with a specific factory implementation,
        // that allows alternative task/connection implementations to be
        // created. This is mostly for testing purposes.
        Engine(std::unique_ptr<Factory> factory,
            bool enable_single_thread_debug);

        // create the engine using internal default factory.
        Engine(const std::string& logpath);
       ~Engine();

        // Perform an account testing to see if the setup is working.
        void TryAccount(const ui::Account& account);

        // Set or modify an account. If the account by the same
        // id already exists then the existing account is modified
        // and connections (if any) might be restarted.
        void SetAccount(const ui::Account& account, bool spawn_connections_immediately = false);

        // Delete the account identified by the id.
        void DelAccount(std::size_t id);

        // Specify the account id to be used as fill account.
        // The account should have been set through to a call to set_account.
        // The fill account is used (if it's set) to fill tasks which are
        // fillable and which have unavailable content on the main server.
        void SetFillAccount(std::size_t id);

        // download the files included in the dowload.
        // all the files are grouped together into a single batch.
        TaskId DownloadFiles(const ui::FileBatchDownload& batch, bool priority = false);

        // Download newsgroup listing.
        TaskId DownloadListing(const ui::GroupListDownload& listing);

        // Download newsgroup headers.
        // If the newsgroup already exists then the headers are updated
        // by downloading the newest headers since the latest update.
        // if there are older headers that have not yet been downloaded
        // then proceed to download those after the newest.
        TaskId DownloadHeaders(const ui::HeaderDownload& update);

        // process pending actions in the engine. You should call this function
        // as a response to to the async_notify.
        // returns true if there are still pending actions to be completed later
        // or false if the pending action queue is empty.
        bool Pump();

        // service engine periodically to perform activities such as
        // reconnect after a specific timeout period.
        // the client should call this function at some steady interval
        // such as 1s or so.
        void Tick();

        // Run main thread actions. This is mostly for supporting
        // single threaded debugging.
        void RunMainThread();

        // start engine. this will start connections and being processing the
        // tasks currently queued in the tasklist.
        void Start();

        // stop the engine. kill all connections and stop all processing.
        void Stop();

        // Load existing engine session from the given file.
        void SaveSession(const std::string& file);

        // Save the current engine session into the given file.
        void LoadSession(const std::string& file);

        // Set the error callback. Invoked on SystemError.
        void SetErrorCallback(on_error error_callback);

        // Set the file callback. Invoked when FileDownloadResult is ready.
        void SetFileCallback(on_file file_callback);

        // Set the batch callback. Invoked when FileBatchResult is ready.
        void SetBatchCallback(on_batch batch_callback);

        // Set the list callback. Invoked when GroupListResult is ready.
        void SetListCallback(on_list list_callback);

        // Set the task callback. Invoked when a task is ready.
        void SetTaskCallback(on_task task_callback);

        // todo
        void SetUpdateCallback(on_update update_callback);

        // set the notify callback.
        void SetNotifyCallback(on_async_notify notify_callback);

        // set the header update callback.
        void SetHeaderInfoCallback(on_header_update callback);

        // Set the finish callback. Invoked when all previously pending tasks are complete.
        void SetFinishCallback(on_finish callback);

        // Set the quota callback. Invoked when downloads progress and data
        // is downloaded from the server defined by the account.
        // The callback can be used to track the account quota changes.
        void SetQuotaCallback(on_quota callback);

        // todo:
        void SetTestCallback(on_conn_test callback);
        void SetTestLogCallback(on_conn_test_log callback);

        // if set to true engine will overwrite files that already exist in the filesystem.
        // otherwise file name collisions are resolved by some naming scheme
        void SetOverwriteExistingFiles(bool on_off);

        // if set engine discards any textual data and only keeps binary content.
        void SetDiscardTextContent(bool on_off);

        // if set engine will prefer secure SSL/TCP connections
        // instead of basic TCP connections whenever secure servers
        // are enabled for the given account.
        void SetPreferSecure(bool on_off);

        // set throttle. If throttle is true then download speed is capped
        // at the given throttle value. (see set_throttle_value)
        void SetEnableThrottle(bool on_off);

        // set the maximum number of bytes to be tranferred in seconds
        // by all engine connections.
        void SetThrottleValue(unsigned value);

        // if set to true tasklist actions perform actions on batches
        // instead of individual tasks. this includes kill/pause/resume
        // and update_task_list
        void SetGroupItems(bool on_off);

        // getters
        bool GetGroupItems() const;
        bool GetOverwriteExistingFiles() const;
        bool GetDiscardTextContent() const;
        bool GetPreferSecure() const;
        bool GetEnableThrottle() const;

        // get current throttle value.
        unsigned GetThrottleValue() const;

        // get how many bytes are currently queued in the engine for downloading.
        // if there are no items this will be 0.
        std::uint64_t GetCurrentQueueSize() const;

        // get how many bytes have been completed of the items that were queued.
        // note that this is a historical value and the items may be removed
        // from the queue. once the queue becomes empty this value drops to 0 as well.
        std::uint64_t GetBytesReady() const;

        // get the total number of bytes written to disk.
        std::uint64_t GetTotalBytesWritten() const;

        // get the total number of bytes downloaded from all the servers/accounts.
        // this value is the number of bytes coming accross the TPC/SSL transport layer
        // and includes a few bytes of protocol data per transaction.
        std::uint64_t GetTotalBytesDownloaded() const;

        // Get the complete path (including the file name) to the engine's log file.
        std::string GetLogfileName() const;

        // get whether engine is started or not.
        bool IsStarted() const;

        // update the tasklist to contain the latest UI states
        // of all the tasks in the engine.
        void GetTasks(std::deque<ui::TaskDesc>* tasklist) const;

        // get the status of a single task at the given index.
        void GetTask(std::size_t index, ui::TaskDesc* task) const;

        // update the connlist to contain the latest UI states
        // of all the connections in the engine.
        void GetConns(std::deque<ui::Connection>* connlist) const;

        // get the connection status of a single connection at the given index.
        void GetConn(std::size_t index, ui::Connection* conn) const;

        // kill the connection at the given index.
        void KillConnection(std::size_t index);

        // clone the connection at the given connection list index.
        void CloneConnection(std::size_t index);

        // kill the task at the given task list index
        TaskId KillTask(std::size_t index);

        // pause the task at the given taks list index.
        TaskId PauseTask(std::size_t index);

        // resume the task at the given task list index.
        TaskId ResumeTask(std::size_t index);

        void MoveTaskUp(std::size_t index);

        void MoveTaskDown(std::size_t index);

        bool KillTaskById(TaskId id);

        bool LockTaskById(TaskId id);

        bool UnlockTaskById(TaskId id);

        // get the number of tasks.
        std::size_t GetNumTasks() const;

        std::size_t GetNumPendingTasks() const;

    private:
        class TaskState;
        class ConnState;
        class ConnTestState;
        class BatchState;
        struct State;

    private:
        std::unique_ptr<State> state_;
    };

    void initialize();

} // newsflash

