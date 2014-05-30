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

#include <newsflash/workaround.h>

#include <corelib/connection.h>
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
#include <corelib/bodylist.h>
#include <corelib/buffer.h>
#include <algorithm>
#include <functional>
#include <vector>
#include <thread>
#include <queue>
#include "task_state_machine.h"
#include "engine.h"
#include "connection.h"
#include "listener.h"
#include "account.h"
#include "server.h"
#include "error.h"
#include "task.h"
#include "file.h"


namespace {
    using namespace newsflash;

    corelib::threadpool threads(5);

    const char* stringify(task::action action)
    {
        switch (action)
        {
            case task::action::none:
                 return "NONE";
            case task::action::prepare:
                return "PREPARE";
            case task::action::flush:
                return "FLUSH";
            case task::action::process:
                return "PROCESS";
            case task::action::cancel:
                return "CANCEL";                
            case task::action::finalize:
                return "FINALIZE";      
            case task::action::kill:
                return "KILL";                          
        }
        assert(0);
        return "???";
    }

    const char* stringify(task::state state)
    {
        switch (state)
        {
            case task::state::queued:
                return "QUEUED";
            case task::state::waiting:
                return "WAITING";
            case task::state::active:
                return "ACTIVE";
            case task::state::debuffering:
                return "DEBUFFERING";
            case task::state::paused:
                return "PAUSED";
            case task::state::complete:
                return "COMPLETE";            
            case task::state::killed:
                return "KILLED";                                                                        
        }
        assert(0);
        return "???";
    }



} // namespace

namespace newsflash
{
struct engine::task_t {
    newsflash::task state;
    corelib::etacalc eta;
    corelib::stopwatch timer;
    corelib::threadpool::tid_t tid;
    std::unique_ptr<corelib::task> task;
    std::unique_ptr<corelib::cmdlist> cmds;
    task_state_machine stm;

    std::size_t thread_submits;

    task_t(const newsflash::task& state) : state(state), eta(0), thread_submits(0)
    {
        LOG_D("Task ", state.id, " created '", state.desc, "'");
    }

   ~task_t()
    {
        assert(stm.is_killed());
        assert(thread_submits == 0);

        LOG_D("Task ", state.id, " deleted");
    }

    static
    std::size_t get_next_id()
    {
        static std::size_t id = 1;
        return id++;
    }
};

struct engine::conn_t {
    connection state;
    corelib::speedometer meter;
    std::unique_ptr<corelib::connection> conn;

    conn_t(const connection& state) : state(state)
    {
        LOG_D("Connection ", state.id, " created");
    }

   ~conn_t()
    {
        LOG_D("Connection ", state.id, " deleted");
    }

    static
    std::size_t get_next_id()
    {
        static std::size_t id = 1;
        return id++;
    }

};

struct engine::batch_t {
    newsflash::task state;
    
    batch_t(const newsflash::task& state) : state(state)
    {
        //LOG_D("Batch created '", state.description, "' (", state.id, ")");
    }

   ~batch_t()
    {
        //LOG_D("Batch deleted ", state.id);        
    }

    static
    std::size_t get_next_id()
    {
        static std::size_t id = 1;
        return id++;
    }
};

engine::engine(listener& callback, std::string logs) : logs_(logs), listener_(callback), stop_(true)
{
    bytes_downloaded_ = 0;
    bytes_written_    = 0;
    bytes_queued_     = 0;

    const auto path = fs::createpath(logs);
    const auto file = fs::joinpath(path, "engine.log");

    LOG_OPEN(file);
    LOG_D("Engine created");

    const settings default_settings = {
        true,  // overwrite existing
        true, // discard text
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
    LOG_D("Engine deleted");
    LOG_FLUSH();
    LOG_CLOSE();
}

void engine::set(const newsflash::settings& settings)
{
    std::lock_guard<std::mutex> lock(mutex_);

    settings_ = settings;

    LOG_I("Engine settings");
    LOG_I("overwrite_existing_files: ", settings_.overwrite_existing_files);
    LOG_I("discard_text_content: ", settings_.overwrite_existing_files);
    LOG_I("auto_remove_completed: ", settings_.auto_remove_completed);
    LOG_I("prefer_secure: ", settings_.prefer_secure);
    LOG_I("enable_throttle: ", settings_.enable_throttle);
    LOG_I("enable_fillserver: ", settings_.enable_fillserver);
    LOG_I("throttle: ", settings_.throttle);
}

void engine::set(const newsflash::account& account)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const newsflash::account& acc)
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
        LOG_D("Restarting forbidden connections");
        for (auto& conn : conns_)
        {
            if (conn->state.account != account.id)
                continue;
            if (conn->state.st != connection::state::error)
                continue;
            if (conn->state.err != connection::error::authentication_failed)
                continue;

            // LOG_D("Restarting ", conn->state.id);

            // bool secure  = false;
            // auto* server = &account.general;
            // if (settings_.prefer_secure && is_valid(account.secure))
            // {
            //     secure = true;
            //     server = &account.secure;
            // }
            // //conn->conn.connect(server->host, server->port, secure);
        }
    }

    LOG_I("Updating account ", account.name);

    acc = account;
}



void engine::pump()
{
    msglib::queue<message>::message msg;
    while (messages_.get(msg))
    {
        auto& lambda = msg.value;

        lambda();

        msg.dispose();
    }
}

void engine::stop()
{
    if (stop_)
        return;

    LOG_I("Stopping engine...");

    // kill all the connections. if the connections
    // are driving tasks, the tasks will transition to
    // appropriate states.
    for (auto& conn : conns_)
    {
        conn->conn->cancel();
        conn->conn.release();
    }

    stop_ = true;
}


void engine::start()
{
    if (!stop_)
        return;

    LOG_I("Starting engine");

    for (auto& account : accounts_)
    {
        start_next_task(account.id);
        start_connections(account.id);
    }

    stop_ = false;
}


void engine::download(const newsflash::account& account, const newsflash::file& file)
{
    set(account);

    newsflash::task state;
    state.err        = task::error::none;
    state.st         = task::state::queued;
    state.act        = task::action::none;
    state.id         = 0; //batch_id;
    state.account    = account.id;
    state.batch      = 0; //batch_id;
    state.desc       = file.desc;
    state.size       = file.size;
    state.runtime    = 0;
    state.eta        = 0;
    state.completion = 0.0;
    state.damaged    = false;

    const auto batch_id = batch_t::get_next_id();
    state.id    = batch_id;
    state.batch = batch_id;
    auto batch  = std::unique_ptr<batch_t>(new batch_t(state));

    const auto task_id  = task_t::get_next_id();
    state.batch = batch_id;
    state.id    = task_id;
    auto task = std::unique_ptr<task_t>(new task_t(state));

    task->stm.on_start  = std::bind(&engine::on_task_start, this, task.get(), batch.get());
    task->stm.on_stop   = std::bind(&engine::on_task_stop, this, task.get(), batch.get());
    task->stm.on_action = std::bind(&engine::on_task_action, this,
        task.get(), batch.get(), std::placeholders::_1);
    task->stm.on_state  = std::bind(&engine::on_task_state, this,
        task.get(), batch.get(), std::placeholders::_1, std::placeholders::_2);

    // for download we know the number of expected buffers
    // so we can configure the state machine right away.
    task->stm.prepare(file.articles.size());

    task->tid = threads.allocate();

    // configure the download
    auto download = std::unique_ptr<corelib::download>(new corelib::download(file.path, file.name));
    download->overwrite(settings_.overwrite_existing_files);
    download->keeptext(!settings_.discard_text_content);

    // configure bodylist
    auto bodylist = std::unique_ptr<corelib::bodylist>(new corelib::bodylist(file.groups, file.articles));
    bodylist->on_body = std::bind(&engine::on_task_body, this,
        task.get(), batch.get(), std::placeholders::_1);

    task->task = std::move(download);
    task->cmds = std::move(bodylist);

    batches_.push_back(std::move(batch));
    tasks_.push_back(std::move(task));
}

void engine::download(const newsflash::account& account, const std::vector<newsflash::file>& files)
{
    set(account);

    newsflash::task batch_state;
    batch_state.err        = task::error::none;
    batch_state.st         = task::state::queued;
    batch_state.act        = task::action::none;
    batch_state.id         = batch_t::get_next_id();
    batch_state.account    = account.id;    
    batch_state.batch      = batch_state.id;
    batch_state.desc       = files[0].desc;
    batch_state.runtime    = 0;
    batch_state.eta        = 0;
    batch_state.completion = 0.0;
    batch_state.damaged    = false;    
    batch_state.size       = std::accumulate(std::begin(files), std::end(files), std::uint64_t(0), 
        [](std::uint64_t size, const newsflash::file& file) {
            return size + file.size;
        });


    auto batch = std::unique_ptr<batch_t>(new batch_t(batch_state));

    for (auto& file : files)
    {
        newsflash::task state;
        state.err        = task::error::none;
        state.st         = task::state::queued;
        state.act        = task::action::none;
        state.id         = task_t::get_next_id();
        state.account    = account.id;
        state.batch      = batch_state.id;
        state.desc       = file.desc;
        state.size       = file.size;
        state.runtime    = 0;
        state.eta        = 0;
        state.completion = 0.0;
        state.damaged    = false;

        auto task = std::unique_ptr<task_t>(new task_t(state));

        task->stm.on_action = std::bind(&engine::on_task_action, this,
            task.get(), batch.get(), std::placeholders::_1);
        task->stm.on_state = std::bind(&engine::on_task_state, this,
            task.get(), batch.get(), std::placeholders::_1, std::placeholders::_2);
        tasks_.push_back(std::move(task));
    }

    batches_.push_back(std::move(batch));
}

void engine::on_task_start(task_t* task, batch_t* batch)
{
    LOG_D("Task ", task->state.id, " START");

    // todo: start timers and eta
    // todo: deal with the batch 

    // find ready connections and make them run the tasks cmdlist
    for (auto& c : conns_)
    {
        if (c->state.account != task->state.account)
            continue;
        if (c->state.task)
            continue;

        auto& cmds = task->cmds;
        auto& conn = c->conn;
        conn->execute(cmds.get());
    }
}

void engine::on_task_stop(task_t* task, batch_t* batch)
{
    LOG_D("Task ", task->state.id, " STOP");

    // todo: stop timers and eta
    // todo: deal with the batch

    for (auto& c : conns_)
    {
        if (c->state.account != task->state.account)
            continue;
        if (!c->state.task)
            continue;

        auto& conn = c->conn;
        conn->cancel();
    }
}



void engine::on_task_action(task_t* task, batch_t*, task::action action)
{
    LOG_D("Task ", task->state.id, " ", stringify(action));

    // delegate the apicall to a thread inside the threadpool.
    // the task is queued for the thread with the affinity to the task

    using a = task::action;

    task->thread_submits++;

    threads.submit(
        [=]() 
        {
            if (action == a::kill)
            {
                messages_.post(
                    [&]() 
                    {
                        auto it = std::find_if(std::begin(tasks_), std::end(tasks_),
                            [&] (const std::unique_ptr<task_t>& maybe) {
                                return maybe.get() == task;
                            });
                        ASSERT(it != std::end(tasks_));
                        tasks_.erase(it);
                    });
                listener_.notify();
                return;
            }

            messages_.post(
                [&]() 
                {
                    task->state.act = action;
                });

            auto& object = task->task;
            try
            {
                switch (action)
                {
                    case a::none: 
                    case a::process:
                    case a::kill:
                        break;

                    case a::prepare:
                        object->prepare();
                        break;

                    case a::flush:
                        object->flush();
                        break;

                    case a::cancel:
                        object->cancel();
                        break;

                    case a::finalize:
                        object->finalize();
                        break;
                }

                // post back success notification 
                messages_.post(
                    [&]() 
                    {
                        task->state.act = a::none;
                        task->thread_submits--;

                        task->stm.complete(action);

                        if (task->stm.is_complete())
                        {
                            start_next_task(task->state.account);                            

                            if (settings_.auto_remove_completed)
                                task->stm.kill();
                        }
                    });
                listener_.notify();
            }
            catch (const std::exception& e)
            {
                // post back failure notification
                messages_.post(
                    [&]() 
                    {
                        task->state.act = a::none;
                        task->thread_submits--;

                        task->stm.fault();

                        if (task->stm.is_complete())
                        {
                            start_next_task(task->state.account);

                            if (settings_.auto_remove_completed)
                                task->stm.kill();
                        }
                    });
                listener_.notify();
            }
        }, task->tid);
}

void engine::on_task_state(task_t* task, batch_t* batch, task::state current,  task::state next)
{
    LOG_D("Task ", task->state.id, " state change ", stringify(current), " => ", stringify(next));


    task->state.st = next;

    // todo: deal with batch shit

    LOG_FLUSH();    
}

// c++11 lambdas do not support captures on movable types
// so we implement things the "old" way for moving buffers
// into tasks.
struct engine::process_buffer : corelib::threadpool::work 
{
    engine* self;
    task_t* task;
    corelib::buffer buff;
    std::size_t id;

    virtual void execute() NOTHROW override 
    {
        auto& messages = self->messages_;
        auto& listener = self->listener_;

        messages.post(
        [&]()
        {
            task->state.act = task::action::process;
        });
        listener.notify();

        const auto bytes = buff.size();

        auto& object = task->task;                
        try 
        {
            object->receive(std::move(buff), id);
            messages.post(
            [&]()
            {
                task->state.act = task::action::none;
                task->thread_submits--;
                task->stm.dequeue(bytes);
            });
        }
        catch (const std::exception& e)
        {
            messages.post(
            [&]() 
            {
                task->state.act = task::action::none;
                task->thread_submits--;
                task->stm.dequeue(bytes);
                task->stm.fault();
                if (task->stm.is_complete())
                {
                    self->start_next_task(task->state.account);
                    if (self->settings_.auto_remove_completed)
                        task->stm.kill();
                }
            });
        }
        listener.notify();
    }
};

void engine::on_task_body(task_t* task, batch_t* batch, corelib::bodylist::body&& body)
{
    using status = corelib::bodylist::status;

    if (body.status == status::success)
    {
        const auto bytes = body.buff.size();
        const auto tid = task->state.id;

        if (task->stm.enqueue(bytes))
        {
            LOG_D("Task ", tid, " enqueue ", bytes, " bytes");            

            auto work  = std::unique_ptr<process_buffer>(new process_buffer);
            work->self = this;
            work->id   = body.id;
            work->buff = std::move(body.buff);
            work->task = task;

            task->thread_submits++;
            threads.submit(std::move(work), task->tid);
        }
        else
        {
            LOG_D("Task ", tid, " dropped ", bytes, " bytes");
            LOG_D("Task ", tid, " current state: ", stringify(task->state.st));
        }
    }
    else if (body.status == status::dmca)
    {
        // todo:
    }
    else if (body.status == status::unavailable)
    {
        // todo:
    }
}

void engine::on_task_xover(task_t* task, batch_t* batch, corelib::xoverlist::xover&& xover)
{
    const auto bytes = xover.buff.size();
    const auto tid   = task->state.id;

    if (task->stm.enqueue(bytes))
    {
        LOG_D("Task ", tid, " enqueue ", bytes, " bytes");

        auto work  = std::unique_ptr<process_buffer>(new process_buffer);
        work->self = this;
        work->id   = 0; // ordering doesn't matter for xover data.
        work->buff = std::move(xover.buff);
        work->task = task;

        task->thread_submits++;
        threads.submit(std::move(work), task->tid);
    }
    else
    {
        LOG_D("Task ", tid, " dropped ", bytes, " bytes ");
        LOG_D("Task ", tid, " current state: ", stringify(task->state.st));
    }
}

void engine::on_task_xover_prepare(task_t* task, batch_t* batch, std::size_t range_count)
{
    const auto tid = task->state.id;

    LOG_D("Task ", tid, " prepared xover ranges ", range_count);

    task->stm.prepare(range_count);
}

void engine::on_conn_ready(conn_t* conn)
{
    messages_.post(
    [=]() 
    {
        LOG_D("Connection ", conn->state.id , " ready");

        conn->state.task  = 0;
        conn->state.bps   = 0;            
        conn->state.err   = connection::error::none;
        conn->state.st    = connection::state::ready;            
        conn->meter.stop();

        // look for a runnable task in the same account
        bool found_runnable_task = false;

        for (auto& task : tasks_)
        {
            if (task->state.account != conn->state.account)
                continue;
            if (!task->stm.is_active() && !task->stm.is_runnable())
                continue;

            LOG_D("Connection ", conn->state.id , " joining task ", task->state.id);

            auto* cmdlist = task->cmds.get();

            // task->stm.join();
            conn->state.task  = task->state.id;
            conn->state.st    = connection::state::active;
            conn->state.desc  = task->state.desc;
            conn->conn->execute(cmdlist);
            found_runnable_task = true;
            break;
        }

        if (!found_runnable_task)
            start_next_task(conn->state.account);
    });
    listener_.notify();
}

void engine::on_conn_error(conn_t* conn, corelib::connection::error error, const std::error_code& system_error)
{
    using i = corelib::connection::error;
    using e = connection::error;

    messages_.post(
    [=]() 
    {
        std::string resource = corelib::format("Connection ", conn->state.id);
        std::string what;

        auto& err = conn->state.err;
        switch (error)
        {
            case i::resolve:
                what = corelib::format("failed to resolve host '", conn->state.host, "'");
                err = e::resolve;
                break;

            case i::refused:
                what = corelib::format("connection to '", conn->state.host, "' refused");
                err = e::refused;
                break;

            case i::forbidden:
                what = corelib::format("authentication failed");
                err = e::authentication_failed;
                break;

            case i::protocol:
                what = corelib::format("nntp protocol error");
                err = e::protocol;
                break;

            case i::ssl:
                what = corelib::format("SSL error");
                err = e::network;                
                break;

            case i::socket:
                what = corelib::format("socket error");
                err = e::network;
                break;

            case i::timeout:
                what = corelib::format("server timeout");
                err = e::timeout;
                break;

            case i::unknown:
                // todo.
                break;
        }

        LOG_E("Connection ", conn->state.id, " '", what, "'");
        if (system_error != std::error_code())
            LOG_E(system_error);

        conn->meter.stop();
        conn->state.bps = 0;
        conn->state.st  = connection::state::error;

        listener_.handle({what, resource, system_error });
    });
    listener_.notify();
}

void engine::on_conn_read(conn_t* conn, std::size_t bytes)
{
    messages_.post(
    [=]() 
    {
        if (!meter_.is_running())
            meter_.start();

        bps_ = meter_.submit(bytes);
        bytes_downloaded_ += bytes;

        if (!conn->meter.is_running())
            conn->meter.start();

        conn->meter.submit(bytes);
        conn->state.down += bytes;
        conn->state.bps = conn->meter.bps();
        conn->state.st  = connection::state::active;
    });
    listener_.notify();
}

void engine::on_conn_auth(conn_t* conn, std::string& user, std::string& pass)
{
    auto* user_ptr = &user;
    auto* pass_ptr = &pass;

    // synchronize the newsflash thread and connection thread.
    // request username and password and wait untill the
    // request has been processed.
    auto rendezvous = messages_.send(
    [=]() 
    {
        LOG_D("Connection ", conn->state.id, " authenticating");

        const auto& account = find_account(conn->state.account);

        *user_ptr = account.username;
        *pass_ptr = account.password;
        conn->state.st = connection::state::authenticating;
    });
    listener_.notify();

    // block the calling connection thread untill the message has been
    // processed by the engine thread.
    rendezvous.wait();
}




void engine::start_next_task(std::size_t account)
{
    if (stop_)
        return;

    // starting at the top of the current task list look for the first
    // currently active task. if we have no active task, then it's time
    // to start the first queued task.
    auto currently_active = std::find_if(std::begin(tasks_), std::end(tasks_),
        [&](const std::unique_ptr<task_t>& task)
        {
            return task->state.account == account && task->stm.is_active();
        });


    // we only permit one task to be currently run by all connections
    // within an account. i.e. all connections work on the same task.
    // if there's an active task already we're done.
    if (currently_active != std::end(tasks_))
    {
        LOG_D("Currently active task: ", (*currently_active)->state.id);
        return;
    }

    // lacking a currently active task, look for the next queued task.
    // we'll make this active if possible.
    auto currently_queued = std::find_if(std::begin(tasks_), std::end(tasks_),
        [&](const std::unique_ptr<task_t>& task) 
        {
            return task->state.account == account && task->stm.is_queued();
        });

    if (currently_queued == std::end(tasks_))
    {
        LOG_D("Have no currently queued task");
        return;
    }

    auto& task = *currently_queued;

    LOG_D("Starting task ", task->state.id);

    // kick it
    task->stm.start();

}

void engine::start_connections(std::size_t account)
{
    const auto& acc = find_account(account);

    const int count = std::count_if(std::begin(conns_), std::end(conns_),
        [=](const std::unique_ptr<conn_t>& conn) {
            return conn->state.account == account;
        });

    if (count == acc.max_connections)
        return;

    LOG_I("Starting connections for '", acc.name, "'");

    for (int i=count; i<acc.max_connections; ++i)
    {
        const auto id = conn_t::get_next_id();
        const auto logfile = fs::joinpath(logs_, corelib::format("connection", id, ".log"));

        const server* host = &acc.general;
        bool ssl = false;

        if (settings_.prefer_secure)
        {
            if (is_valid(acc.secure))
            {
                host = &acc.secure;
                ssl  = true;
            }
        }

        connection state;
        state.err     = connection::error::none;
        state.st      = connection::state::connecting;
        state.id      = id;
        state.task    = 0;
        state.account = account;
        state.down    = 0;
        state.host    = host->host;
        state.secure  = ssl;
        state.bps     = 0;

        std::unique_ptr<conn_t> conn(new conn_t(state));
        std::unique_ptr<corelib::connection> foo(new corelib::connection);

        foo->on_ready = std::bind(&engine::on_conn_ready, this, conn.get());
        foo->on_error = std::bind(&engine::on_conn_error, this, conn.get(),
            std::placeholders::_1, std::placeholders::_2);
        foo->on_read = std::bind(&engine::on_conn_read, this, conn.get(),
            std::placeholders::_1);
        foo->on_auth = std::bind(&engine::on_conn_auth, this, conn.get(),
            std::placeholders::_1, std::placeholders::_2);

        foo->connect(logfile , host->host, host->port, ssl);

        LOG_D("Connection ", id, " (", i+1, "/", acc.max_connections, ")");
    }

}


newsflash::account& engine::find_account(std::size_t id)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const account& acc)
        {
            return acc.id == id;
        });
    ASSERT(it != std::end(accounts_));

    return *it;
}

} // engine
