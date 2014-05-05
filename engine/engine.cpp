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

#include <corelib/connection.h>
#include <corelib/taskstate.h>
#include <corelib/threadpool.h>
#include <corelib/download.h>
#include <corelib/logging.h>
#include <corelib/assert.h>
#include <corelib/filesys.h>
#include <corelib/waithandle.h>
#include <corelib/speedometer.h>
#include <corelib/throttle.h>
#include <corelib/format.h>
#include <corelib/etacalc.h>
#include <corelib/cmdlist.h>
#include <corelib/stopwatch.h>
#include <algorithm>
#include <functional>
#include <vector>
#include <thread>
#include "engine.h"
#include "connection.h"
#include "listener.h"
#include "account.h"
#include "server.h"
#include "task.h"
#include "file.h"

namespace {
    corelib::threadpool threads(5);

    const char* stringify(corelib::taskstate::action action)
    {
        using namespace corelib;

        switch (action)
        {
            case taskstate::action::prepare:
                return "PREPARE";
            case taskstate::action::flush:
                return "FLUSH";
            case taskstate::action::cancel:
                return "CANCEL";                
            case taskstate::action::finalize:
                return "FINALIZE";                                
            case taskstate::action::run_cmd_list:
                return "RUN";                                            
            case taskstate::action::stop_cmd_list:
                return "STOP";                                                            
        }
        assert(0);
        return "???";
    }

    const char* stringify(corelib::taskstate::state state)
    {
        using namespace corelib;

        switch (state)
        {
            case taskstate::state::queued:
                return "QUEUED";
            case taskstate::state::waiting:
                return "WAITING";
            case taskstate::state::active:
                return "ACTIVE";
            case taskstate::state::paused:
                return "PAUSED";
            case taskstate::state::complete:
                return "COMPLETE";            
            case taskstate::state::killed:
                return "KILLED";                                                                        
        }
        assert(0);
        return "???";
    }

} // namespace

namespace engine
{
struct engine::task_t {
    ::engine::task state;
    corelib::taskstate stm;
    corelib::etacalc eta;
    corelib::stopwatch timer;
    corelib::threadpool::tid_t tid;
    std::unique_ptr<corelib::task> task;
    std::unique_ptr<corelib::cmdlist> cmd;

    task_t(const ::engine::task& state) : state(state), eta(0)
    {
        LOG_D("Task created '", state.description, "' (", state.id, ")");
    }

   ~task_t()
    {
        LOG_D("Task deleted ", state.id);
    }

    static
    std::size_t get_next_id()
    {
        static std::size_t id = 1;
        return id++;
    }
};

struct engine::conn_t {
    ::engine::connection state;
    corelib::connection conn;
    corelib::speedometer speed;

    conn_t(const std::string& logfile) : conn(logfile)
    {
        state.error = ::engine::connection_error::none;
        state.state = ::engine::connection_state::connecting;
        state.id    = 0;
        state.task  = 0;
        state.account = 0;
        state.bytes  = 0;
        state.secure = false;
        state.bps  = 0;

        LOG_D("Connection created ", state.id);
    }

   ~conn_t()
    {
        LOG_D("Connection deleted ", state.id);
    }

    static
    std::size_t get_next_id()
    {
        static std::size_t id = 1;
        return id++;
    }

};

struct engine::batch_t {
    ::engine::task state;
    
    batch_t(const ::engine::task& state) : state(state)
    {
        LOG_D("Batch created '", state.description, "' (", state.id, ")");
    }

   ~batch_t()
    {
        LOG_D("Batch deleted ", state.id);        
    }

    static
    std::size_t get_next_id()
    {
        static std::size_t id = 1;
        return id++;
    }
};

struct engine::msg_conn_ready {
    conn_t* conn;
};

struct engine::msg_conn_error {
    conn_t* conn; 
};

struct engine::msg_conn_read {
    conn_t* conn;
    std::size_t bytes;
};

struct engine::msg_conn_auth {
    conn_t* conn;
    std::string* user;
    std::string* pass;
};

engine::engine(listener& callback, std::string logs) : logs_(logs), listener_(callback)
{
    fs::create_path(logs);

    LOG_OPEN(fs::joinpath(logs, "engine.log"));    

    const settings default_settings = {
        true,  // overwrite existing
        true, // discard text
        true, // auto connect
        true, // auto remove completed
        true, // prefer secure
        false, // throttle
        false, // enable fill
        0
    };

    set(default_settings);
}

engine::~engine()
{
    LOG_CLOSE();
}

void engine::set(const ::engine::settings& settings)
{
    std::lock_guard<std::mutex> lock(mutex_);

    settings_ = settings;

    LOG_I("Current settings");
    LOG_I("overwrite_existing_files: ", settings_.overwrite_existing_files);
    LOG_I("discard_text_content: ", settings_.overwrite_existing_files);
    LOG_I("auto_connect: ", settings_.auto_connect);
    LOG_I("auto_remove_completed: ", settings_.auto_remove_completed);
    LOG_I("prefer_secure: ", settings_.prefer_secure);
    LOG_I("enable_throttle: ", settings_.enable_throttle);
    LOG_I("enable_fillserver: ", settings_.enable_fillserver);
    LOG_I("throttle: ", settings_.throttle);
}

void engine::set(const ::engine::account& account)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const ::engine::account& acc)
        {
            return acc.id == account.id;
        });
    if (it == std::end(accounts_))
    {
        LOG_I("Adding new account ", account.name);
        accounts_.push_back(account);
        return;
    }

    auto& acc = *it;

    // if username/password are modified and there are connections
    // in forbidden state, restart those connections
    if (acc.username != account.username || acc.password != account.password)
    {
        LOG_D("Restarting forbidden connections (if any)");
        for (auto& conn : conns_)
        {
            if (conn->state.account != account.id)
                continue;
            if (conn->state.state != ::engine::connection_state::error)
                continue;
            if (conn->state.error != ::engine::connection_error::forbidden)
                continue;

            LOG_D("Restarting ", conn->state.id);

            bool secure  = false;
            auto* server = &account.general;
            if (settings_.prefer_secure && is_valid(account.secure))
            {
                secure = true;
                server = &account.secure;
            }
            conn->conn.connect(server->host, server->port, secure);
        }
    }

    LOG_I("Updating account ", account.name);

    acc = account;
}



void engine::pump_events()
{

}

void engine::download(const ::engine::account& account, const ::engine::file& file)
{
    set(account);

    const auto batch_id = batch_t::get_next_id();
    const auto task_id  = task_t::get_next_id();

    ::engine::task state;
    state.error       = ::engine::task_error::none;
    state.state       = ::engine::task_state::queued;
    state.id          = batch_id;
    state.account     = account.id;
    state.batch       = batch_id;
    state.description = file.description;
    state.size        = file.size;
    state.runtime     = 0;
    state.eta         = 0;
    state.completion  = 0.0;
    state.damaged     = false;

    auto batch = std::unique_ptr<batch_t>(new batch_t(state));

    state.batch = batch_id;
    state.id    = task_id;
    auto task = std::unique_ptr<task_t>(new task_t(state));

    task->stm.on_action = std::bind(&engine::on_task_action, this,
        task.get(), batch.get(), std::placeholders::_1);
    task->stm.on_state_change = std::bind(&engine::on_task_state_change, this,
        task.get(), batch.get(), std::placeholders::_1, std::placeholders::_2);

    batches_.push_back(std::move(batch));
    tasks_.push_back(std::move(task));

    schedule_tasklist();
    schedule_connlist();
}

void engine::download(const ::engine::account& account, const std::vector<::engine::file>& files)
{
    set(account);

    ::engine::task batch_state;
    batch_state.error       = ::engine::task_error::none;
    batch_state.state       = ::engine::task_state::queued;
    batch_state.id          = batch_t::get_next_id();
    batch_state.account     = account.id;    
    batch_state.batch       = batch_state.id;
    batch_state.description = files[0].description;
    batch_state.size        = std::accumulate(std::begin(files), std::end(files), std::uint64_t(0), 
        [](std::uint64_t size, const ::engine::file& file) {
            return size + file.size;
        });
    batch_state.runtime     = 0;
    batch_state.eta         = 0;
    batch_state.completion  = 0.0;
    batch_state.damaged     = false;

    auto batch = std::unique_ptr<batch_t>(new batch_t(batch_state));

    for (auto& file : files)
    {
        ::engine::task state;
        state.error       = ::engine::task_error::none;
        state.state       = ::engine::task_state::queued;
        state.id          = task_t::get_next_id();
        state.account     = account.id;
        state.batch       = batch_state.id;
        state.description = file.description;
        state.size        = file.size;
        state.runtime     = 0;
        state.eta         = 0;
        state.completion  = 0.0;
        state.damaged     = false;

        auto task = std::unique_ptr<task_t>(new task_t(state));

        task->stm.on_action = std::bind(&engine::on_task_action, this,
            task.get(), batch.get(), std::placeholders::_1);
        task->stm.on_state_change = std::bind(&engine::on_task_state_change, this,
            task.get(), batch.get(), std::placeholders::_1, std::placeholders::_2);
        tasks_.push_back(std::move(task));
    }

    batches_.push_back(std::move(batch));

    schedule_tasklist();
    schedule_connlist();
}



void engine::on_task_action(task_t* task, batch_t*, corelib::taskstate::action action)
{
    LOG_D("Task action (", task->state.id, ") ", stringify(action));

    // perform the action instructed by the state machine
    // we delegate the actual operation to a background
    // thread available in the thread pool and queue
    // the work with the tasks's thread affinity key
    // this way operations happen in a well defined order
    // for each task.

    switch (action)
    {
        case corelib::taskstate::action::run_cmd_list:
            break;
        case corelib::taskstate::action::stop_cmd_list:
            break;

        default:
            threads.submit(
                [=]()
                {
                    auto& object = task->task;
                    try
                    {
                        switch (action)
                        {
                            case corelib::taskstate::action::prepare:
                                object->prepare();
                                break;

                            case corelib::taskstate::action::flush:
                                object->flush();
                                break;

                            case corelib::taskstate::action::cancel:
                                object->cancel();
                                break;

                            case corelib::taskstate::action::finalize:
                                object->finalize();
                                break;

                            default:
                                assert(0);
                                break;
                        }

                    }
                    catch (const std::exception& e)
                    {
                    }
                }, task->tid);
            break;
    }
}

void engine::on_task_state_change(task_t* task, batch_t* batch, corelib::taskstate::state current,  corelib::taskstate::state next)
{
    LOG_D("Task state change (", task->state.id, ") ", stringify(current), " -> ", stringify(next));

    // map the 
}

void engine::on_conn_ready(conn_t* conn)
{
}

void engine::on_conn_error(conn_t* conn, corelib::connection::error error)
{
}

void engine::on_conn_read(conn_t* conn, std::size_t bytes)
{
}

void engine::on_conn_auth(conn_t* conn, std::string& user, std::string& pass)
{

    //auto future = messages_.send(msglib::message(1, msg_conn_auth{conn, &user, &pass}));

    //std::lock_guard<std::mutex> lock(mutex_);
    // const auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
    //     [=](const account& acc)
    //     {
    //         return acc.id == conn->state.id;
    //     });
    // ASSERT(it != std::end(accounts_));

    // const auto& account = *it;

    // user = account.username;
    // pass = account.password;
}

bool engine::on_conn_throttle(conn_t* conn, std::size_t& quota)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!settings_.enable_throttle)
        return false;

    // todo:

    return true;
}


void engine::schedule_tasklist()
{
    for (const auto& account : accounts_)
    {
        // starting at the top of the current task list look for the first
        // currently active task. if we have no active task, then it's time
        // to start the first queued task.
        auto currently_active = std::find_if(std::begin(tasks_), std::end(tasks_),
            [&](const std::unique_ptr<task_t>& task)
            {
                return task->state.account == account.id &&
                    task->stm.is_active();
            });
        // we only permit one task to be currently run by all connections
        // within an account. i.e. all connections work on the same task.
        // if there's an active task already we're done.
        if (currently_active != std::end(tasks_))
            continue;

        // lacking a currently active task, look for the next queued task.
        // we'll make this active if possible.
        auto currently_queued = std::find_if(std::begin(tasks_), std::end(tasks_),
            [&](const std::unique_ptr<task_t>& task) 
            {
                return task->state.account == account.id && 
                    task->stm.is_queued();
            });
        if (currently_queued == std::end(tasks_))
            continue;        

        auto& task = *currently_queued;

        task->stm.start();
    }
}

void engine::schedule_connlist()
{
    for (const auto& account : accounts_)
    {
        const int count = std::count_if(std::begin(conns_), std::end(conns_),
            [=](const std::unique_ptr<conn_t>& conn)
            {
                return conn->state.account == account.id;
            });
        if (count == account.max_connections)
            return;

        for (int i=count; i<account.max_connections; ++i)
        {
            const auto cid = conns_.size() + 1;
            const auto log = corelib::format("connection", cid, ".log");

            //LOG_I("Starting a new connection to ", account.name);

            //std::unique_ptr<conn_t> conn(new conn_t(log));
        }
    }
}

::engine::account* engine::find_account(std::size_t id)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const account& acc)
        {
            return acc.id == id;
        });
    if (it == std::end(accounts_))
        return nullptr;

    return &(*it);
}

} // engine
