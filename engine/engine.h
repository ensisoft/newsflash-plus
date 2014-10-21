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
#include <mutex>
#include <string>
#include <deque>
#include <memory>
#include <vector>
#include <queue>
#include <fstream>
#include <cstdint>

#include "ui/task.h"
#include "ui/connection.h"
#include "ui/download.h"
#include "ui/error.h"
#include "ui/file.h"
#include "threadpool.h"
#include "settings.h"
#include "account.h"

namespace newsflash
{
    class task;
    class connection;
    class action;
    class batch;

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

        template<typename UiType, typename EngineType>
        class list
        {
        public:
            using list_t = std::deque<std::unique_ptr<EngineType>>;

            list(list_t& list) : list_(&list)
            {}

            UiType operator[](std::size_t i) const 
            { return (*list_)[i]->get_ui_state(); }

            std::size_t size() const 
            { return list_->size(); }
        private:
            list_t* list_;
        };

        // list of UI visible engine task states.
        using task_list = list<ui::task, newsflash::task>;

        // list of UI visible engine connection states.
        using conn_list = list<ui::connection, newsflash::connection>;

        engine();
       ~engine();

        void set(const account& acc);

        void download(std::size_t acc, std::vector<ui::download> batch);

        void pump();

        void start();

        void stop();

        void configure(const settings& s);

        void set_log_folder(std::string path);

    private:
        void on_action_complete(action* act);
    private:

        void complete_conn_action(action* act, connection* conn);
        void complete_task_action(action* act, task* task);
        void remove_completed_tasks();
        void begin_next_task();

    private:
        std::deque<std::unique_ptr<task>> tasks_;
        std::deque<std::unique_ptr<connection>> conns_;
        std::vector<account> accounts_;
        std::uint64_t bytes_downloaded_;
        std::uint64_t bytes_queued_;
        std::uint64_t bytes_written_;
        std::mutex mutex_;
        std::queue<action*> actions_;
        std::string logpath_;        
        std::ofstream log_;
    private:
        threadpool threads_;
    private:
        settings settings_;
    private:
        bool started_;
    };
    
} // newsflash

