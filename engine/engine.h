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
#include "ui/account.h"
#include "ui/task.h"
#include "ui/connection.h"
#include "ui/download.h"
#include "ui/error.h"
#include "ui/file.h"
#include "ui/batch.h"
#include "ui/listing.h"
#include "ui/update.h"

namespace newsflash
{
    class engine
    {
    public:
        // this callback is invoked when an error has occurred.
        // the error object carries information and details about 
        // what happened. 
        using on_error = std::function<void (const ui::error& error)>;

        // this callback is invoked when a new file has been completed.
        using on_file = std::function<void (const ui::file& file)>;

        // this callback is invoked when a batch is complete
        using on_batch = std::function<void (const ui::batch& batch)>;

        // this callback is invoked when a listing is complete.
        using on_list  = std::function<void (const ui::listing& listing)>;

        // this callback is invoked when a update is complete
        using on_update = std::function<void(const ui::update& update)>;

        // 
        using on_header_data = std::function<void(const std::string& file)>;

        using on_header_info = std::function<void(const std::string& group,
            std::uint64_t num_articles_local, std::uint64_t num_articles_remote)>;

        // this callback is invoked when there are pending events inside the engine
        // the handler function should organize for a call into engine::pump() 
        // to process the pending events. 
        // note that this callback handler needs to be thread safe and the notifications
        // can come from multiple threads inside the engine.
        using on_async_notify = std::function<void ()>;

        // this callback is called when all tasks are done.
        using on_complete = std::function<void()>;

        // this callback is called when payload data that affects
        // quota is downloaded.
        using on_quota = std::function<void (std::size_t bytes, std::size_t account)>;

        using action_id_t = std::size_t;

        engine();
       ~engine();

        // set or modify an account.
        void set_account(const ui::account& acc);

        // delete the account identified by the id.
        void del_account(std::size_t id);

        void set_fill_account(std::size_t id);

        // download the files included in the dowload.
        // all the files are grouped together into a single batch.
        action_id_t download_files(ui::batch batch);

        action_id_t download_listing(ui::listing list);

        action_id_t download_headers(ui::update update);

        action_id_t get_action_id(std::size_t task_index);

        // process pending actions in the engine. You should call this function
        // as a response to to the async_notify.
        // returns true if there are still pending actions to be completed later
        // or false if the pending action queue is empty.
        bool pump();

        // service engine periodically to perform activities such as 
        // reconnect after a specific timeout period.
        // the client should call this function at some steady interval
        // such as 1s or so.
        void tick();

        // start engine. this will start connections and being processing the
        // tasks currently queued in the tasklist.
        void start(std::string logs);

        // stop the engine. kill all connections and stop all processing.
        void stop();

        void save_session(const std::string& file);

        void load_session(const std::string& file);

        // set the error callback.
        void set_error_callback(on_error error_callback);

        // set the file callback.
        void set_file_callback(on_file file_callback);

        void set_batch_callback(on_batch batch_callback);

        void set_list_callback(on_list list_callback);

        void set_update_callback(on_update update_callback);

        // set the notify callback. this 
        void set_notify_callback(on_async_notify notify_callback);

        void set_header_data_callback(on_header_data callback);
        void set_header_info_callback(on_header_info callback);

        void set_complete_callback(on_complete callback);

        void set_quota_callback(on_quota callback);

        // if set to true engine will overwrite files that already exist in the filesystem.
        // otherwise file name collisions are resolved by some naming scheme
        void set_overwrite_existing_files(bool on_off);

        // if set engine discards any textual data and only keeps binary content.
        void set_discard_text_content(bool on_off);

        // if set engine will prefer secure SSL/TCP connections 
        // instead of basic TCP connections whenever secure servers
        // are enabled for the given account.
        void set_prefer_secure(bool on_off);

        // set throttle. If throttle is true then download speed is capped
        // at the given throttle value. (see set_throttle_value)
        void set_throttle(bool on_off);

        // set the maximum number of bytes to be tranferred in seconds
        // by all engine connections.
        void set_throttle_value(unsigned value);

        // if set to true tasklist actions perform actions on batches
        // instead of individual tasks. this includes kill/pause/resume
        // and update_task_list
        void set_group_items(bool on_off);

        // getters
        bool get_group_items() const;
        bool get_overwrite_existing_files() const;
        bool get_discard_text_content() const;
        bool get_prefer_secure() const;
        bool get_throttle() const;

        // get current throttle value.
        unsigned get_throttle_value() const;

        // get how many bytes are currently queued in the engine for downloading.
        // if there are no items this will be 0.
        std::uint64_t get_bytes_queued() const;

        // get how many bytes have been completed of the items that were queued.
        // note that this is a historical value and the items may be removed
        // from the queue. once the queue becomes empty this value drops to 0 as well.
        std::uint64_t get_bytes_ready() const;

        // get the total number of bytes written to disk.
        std::uint64_t get_bytes_written() const;

        // get the total number of bytes downloaded from all the servers/accounts.
        // this value is the number of bytes coming accross the TPC/SSL transport layer
        // and includes a few bytes of protocol data per transaction.
        std::uint64_t get_bytes_downloaded() const;

        std::string get_logfile() const;

        // get whether engine is started or not.
        bool is_started() const;

        // update the tasklist to contain the latest UI states
        // of all the tasks in the engine.
        void update(std::deque<ui::task>& tasklist);

        // update the connlist to contain the latest UI states
        // of all the connections in the engine.
        void update(std::deque<ui::connection>& connlist);

        // kill the connection at the given index.
        void kill_connection(std::size_t index);

        // clone the connection at the given connection list index.
        void clone_connection(std::size_t index);

        // kill the task at the given task list index
        void kill_task(std::size_t index);

        // pause the task at the given taks list index.
        void pause_task(std::size_t index);

        // resume the task at the given task list index.
        void resume_task(std::size_t index);
        
        void move_task_up(std::size_t index);

        void move_task_down(std::size_t index);

        void kill_action(action_id_t id);

        // get the number of tasks.
        std::size_t num_tasks() const;

        std::size_t num_pending_tasks() const;

    private:
        class task;
        class conn;
        class batch;
        struct state;

    private:
        std::shared_ptr<state> state_;
    };
    
} // newsflash

