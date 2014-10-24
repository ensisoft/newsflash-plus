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
#include "bitflag.h"

namespace newsflash
{
    class account;

    class engine
    {
    public:
        // this callback is invoked when an error has occurred.
        // the error object carries information and details about 
        // what happened. 
        std::function<void (const ui::error& error)> on_error;

        // this callback is invoked when a new file has been completed.
        std::function<void (const ui::file& file)> on_file;

        // this callback is invoked when there are pending events inside the engine
        // the handler function should organize for a call into engine::pump() 
        // to process the pending events. 
        // note that this callback handler needs to be thread safe and the notifications
        // can come from multiple threads inside the engine.
        std::function<void ()> on_async_notify;

        engine();
       ~engine();

        void set(const account& acc);

        void del(const account& acc);

        void download(ui::download spec);

        // process pending actions in the engine. You should call this function
        // as a response to to the async_notify.
        void pump();

        // start engine. this will start connections and being processing the
        // tasks currently queued in the tasklist.
        void start(std::string logs);

        // stop the engine. kill all connections and stop all processing.
        void stop();

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

        // get overwrite value
        bool get_overwrite_existing_files() const;

        // get discard value
        bool get_discard_text_content() const;

        // get secure value
        bool get_prefer_secure() const;

        bool get_throttle() const;

        // get whether engine is started or not.
        bool is_started() const;

        // update the tasklist to contain the latest UI states
        // of all the tasks in the engine.
        void update(std::deque<const ui::task*>& tasklist);

        // update the connlist to contain the latest UI states
        // of all the connections in the engine.
        void update(std::deque<const ui::connection*>& connlist);

    private:
        void begin_connect(std::size_t account);

    private:
        class task;
        class conn;
        class batch;
        struct state;

    private:
        std::shared_ptr<state> state_;
    };
    
} // newsflash

