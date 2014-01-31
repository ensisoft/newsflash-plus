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

#include <algorithm>
#include <functional>
#include <thread>
#include <vector>
#include <deque>

#include "engine.h"
#include "listener.h"
#include "connection.h"
#include "command.h"
#include "logging.h"
#include "task.h"
#include "event.h"
#include "msgqueue.h"
#include "assert.h"

namespace {

} // namespace

namespace newsflash
{

class engine::impl
{
public:
    impl(const std::string& logs, const engine::configuration& config, newsflash::listener* listener) 
        : logfolder_(logs)
        , config_(config)
        , listener_(listener)
        , thread_(std::bind(&engine::impl::main, this))
    {}

   ~impl()
    {}

    void join()
    {
        thread_.join();
    }

    void shutdown()
    {

    }


    void add_account(const engine::account& acc)
    {

    }

private:
    void handle_messages()
    {
        std::unique_ptr<msgqueue::message> msg;

        while (messages_.try_get_front(msg))
        {

        }
    }

    void dispatch_responses()
    {
        // response resp;
        // while (responses_.try_get_one(resp))
        // {
        //     const auto it = std::find_if(tasks_.begin(), tasks_.end(),
        //         [&](const task& t)
        //         {
        //             return t.id() == resp.taskid;
        //         });

        //     ASSERT(it != tasks_.end());
        //     //const auto id = it
        // }
    }

    void remove_tasks()
    {
        if (tasks_.empty())
            return;

        // std::remove_if(tasks_.begin(), tasks_.end(),
        //     [&](const task& t)
        //     {
        //         const task::state state = t.get_state();
        //         return state == task::state::done ||
        //                state == task::state::killed;
        //     });

    }

    void remove_connections()
    {
        if (conns_.empty())
            return;


    }

    void schedule_tasks()
    {}

    void schedule_connections()
    {

    }

    void loop()
    {
        const std::chrono::milliseconds scheduling_interval(100);

        while (true)
        {
            auto msg_handle = messages_.wait();
            auto res_handle = responses_.wait();

            if (!wait(msg_handle, res_handle, scheduling_interval))
            {
                remove_tasks();
                remove_connections();
                schedule_tasks();
                schedule_connections();
            }
            else if (msg_handle.read())
            {
                handle_messages();
            }
            else if (res_handle.read())
            {
                dispatch_responses();
            }
        }
    }

    void main()
    {
        LOG_OPEN(logfolder_ + "/engine.log");

        LOG_D("Engine thread: ", std::this_thread::get_id());
        try
        {
            loop();
        }
        catch (const std::exception& e)
        {
            LOG_E(e.what());
        }
        LOG_D("Engine thread has exited.");
    }

private:
    typedef std::deque<task*> tasklist;
    typedef std::deque<connection*> connlist;    
    typedef std::vector<engine::account> acclist;

private:
    const std::string& logfolder_;
    engine::configuration config_;

    listener* listener_;    
    msgqueue messages_;
    cmdqueue commands_;
    resqueue responses_;
    tasklist tasks_;
    connlist conns_;
    acclist  accounts_;

    std::mutex mutex_;    
    std::thread thread_;    
};


engine::engine(listener* list, const configuration& config, 
    const std::string& logfolder)
{
    pimpl_.reset(new engine::impl(logfolder, config, list));
}

engine::engine(listener* list, const configuration& config)
{
    pimpl_.reset(new engine::impl("", config, list));
}


engine::~engine()
{
    pimpl_->join();
}

void engine::shutdown()
{
    pimpl_->shutdown();
}

void engine::add_account(const account& acc)
{
    pimpl_->add_account(acc);
}

} // newsflash

