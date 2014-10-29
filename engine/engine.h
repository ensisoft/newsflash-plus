// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#include <functional>
#include <memory>
#include <deque>
#include "ui/task.h"
#include "ui/connection.h"
#include "ui/download.h"
#include "ui/error.h"
#include "ui/file.h"
#include "account.h"

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

        // this callback is invoked when there are pending events inside the engine
        // the handler function should organize for a call into engine::pump() 
        // to process the pending events. 
        // note that this callback handler needs to be thread safe and the notifications
        // can come from multiple threads inside the engine.
        using on_async_notify = std::function<void ()>;

        engine();
       ~engine();

        void set_account(const account& acc);

        void del_account(std::size_t id);

        void download(ui::download spec);

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

        void set_error_callback(on_error error_callback);

        void set_file_callback(on_file file_callback);

        void set_notify_callback(on_async_notify notify_callback);

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

        void set_group_items(bool on_off);

        bool get_group_items() const;

        // get overwrite value
        bool get_overwrite_existing_files() const;

        // get discard value
        bool get_discard_text_content() const;

        // get secure value
        bool get_prefer_secure() const;

        bool get_throttle() const;

        unsigned get_throttle_value() const;

        // get whether engine is started or not.
        bool is_started() const;

        // update the tasklist to contain the latest UI states
        // of all the tasks in the engine.
        void update(std::deque<ui::task>& tasklist);

        // update the connlist to contain the latest UI states
        // of all the connections in the engine.
        void update(std::deque<ui::connection>& connlist);

        void kill_connection(std::size_t index);

        void clone_connection(std::size_t index);

        void kill_task(std::size_t index);

        void pause_task(std::size_t index);

        void resume_task(std::size_t index);


    private:
        void connect();

    private:
        class task;
        class conn;
        class batch;
        struct state;

    private:
        std::shared_ptr<state> state_;
    };
    
} // newsflash

