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

#include <newsflash/config.h>

#include <algorithm>
#include <deque>
#include <mutex>
#include <fstream>
#include <cassert>

#include "engine.h"
#include "connection.h"
#include "download.h"
#include "action.h"
#include "assert.h"
#include "filesys.h"
#include "logging.h"
#include "utf8.h"
#include "threadpool.h"

namespace newsflash
{

struct engine::state {
    std::deque<std::unique_ptr<engine::task>> tasks;
    std::deque<std::unique_ptr<engine::conn>> conns;
    std::deque<std::unique_ptr<engine::batch>> batches;
    std::vector<account> accounts;
    std::size_t bytes_downloaded;
    std::size_t bytes_queued;
    std::size_t bytes_written;
    std::size_t oid; // object id for tasks/connections
    std::string logpath;
    std::ofstream log;

    std::mutex  mutex;
    std::queue<action*> actions;
    std::unique_ptr<threadpool> threads;

    bool prefer_secure;
    bool overwrite_existing;
    bool discard_text;
    bool started;

    const account& find_account(std::size_t id) const 
    {
        auto it = std::find_if(accounts.begin(), accounts.end(),
            [=](const account& a) {
                return a.id == id;
            });
        if (it == std::end(accounts))
            throw std::runtime_error("no such account");

        return *it;
    }

    bool start(std::string logs)
    {
        if (started)
            return false;

        log.open(fs::joinpath(logs, "engine.log"), std::ios::trunc);
        if (log.is_open())
            set_thread_log(&log);
        else set_thread_log(nullptr);

        logpath = logs;
        started = true;
        return true;
    }
};


class engine::conn
{
public:
    using state = ui::connection::states;
    using error = ui::connection::errors;

    conn(std::size_t account, engine::state& s) : state_(s)
    {
        const auto& acc = s.find_account(account);

        connection::spec spec;
        spec.password = acc.password;
        spec.username = acc.username;
        if (s.prefer_secure && acc.enable_secure_server)
        {
            spec.hostname = acc.secure_host;
            spec.hostport = acc.secure_port;
            spec.use_ssl  = true;
            ui_.secure    = true;
            ui_.host      = acc.secure_host;
            ui_.port      = acc.secure_port;
        }
        else
        {
            spec.hostname = acc.general_host;
            spec.hostport = acc.general_port;
            spec.use_ssl  = false;
            ui_.secure    = false;
            ui_.host      = acc.general_host;
            ui_.port      = acc.general_port;
        }

        ui_.account = account;
        ui_.bps     = 0;
        ui_.desc    = "";
        ui_.error   = error::none;
        ui_.state   = state::disconnected;
        ui_.down    = 0;
        ui_.id      = s.oid++;

        auto action = conn_.connect(spec);        
        do_action(std::move(action));

        LOG_I("Connection ", ui_.id, " ", ui_.host, ":", ui_.port);

        log_ = std::make_shared<std::fstream>();
        log_->open(fs::joinpath(s.logpath, "connection.log"),
            std::ios::trunc);
    }

   ~conn()
    {
        LOG_I("Deleted connection ", ui_.id);
    }

    void update(const ui::connection*& ui)
    {
        ui = &ui_;
    }
    void on_action(action* a)
    {
        assert(a->get_id() == ui_.id);

        if (a->has_exception())
        {
            try
            {
                a->rethrow();
            }
            catch (const std::exception &e)
            {
                ui_.state = state::error;
                ui_.error = error::other;                
            }
            return;
        }

        auto next = conn_.complete(std::unique_ptr<action>(a));
        if (next)
        {
            do_action(std::move(next));
            return;
        }
        
        if (ui_.state == state::initializing)
            ui_.state = state::connected;
        else if (ui_.state == state::active)
            ui_.state = state::connected;
    }

    std::size_t account() const
    { return ui_.account; }

    std::size_t id() const 
    { return ui_.id; }

private:
    void do_action(std::unique_ptr<action> a)
    {
        auto* ptr = a.get();

        if (dynamic_cast<class connection::resolve*>(ptr))
            ui_.state = state::resolving;
        else if (dynamic_cast<class connection::connect*>(ptr))
            ui_.state = state::connecting;
        else if (dynamic_cast<class connection::initialize*>(ptr))
            ui_.state = state::initializing;
        else if (dynamic_cast<class connection::execute*>(ptr))
            ui_.state = state::active;

        a->set_id(ui_.id);
        a->set_log(log_);
        state_.threads->submit(a.release());
    }

private:
    ui::connection ui_;
    connection conn_;
    engine::state& state_;
    std::shared_ptr<std::fstream> log_;
private:
    static unsigned NUMBER;
};

class engine::task 
{
public:
    using state = ui::task::states;

    task(std::size_t account, std::uint64_t size, std::string desc,
        std::unique_ptr<newsflash::task> task, engine::state& s) : task_(std::move(task)), state_(s)
    {
        ui_.state      = state::queued;
        ui_.id         = state_.oid++;
        ui_.account    = account;
        ui_.desc       = desc;
        ui_.size       = size;
        ui_.runtime    = 0;
        ui_.etatime    = 0;
        ui_.completion = 0.0;

        LOG_I("Created task ", ui_.id);
    }

   ~task()
    {
        LOG_I("Deleted task ", ui_.id);
    }

    bool start(conn* c)
    {
        return false;
    }

    void pause()
    {
    }

    void update(const ui::task*& ui)
    {
        ui = &ui_;
    }

    void on_action(action* a)
    {

    }

    std::size_t account() const 
    { return ui_.account; }

    std::size_t id() const
    { return ui_.id; }

private:
    ui::task ui_;    
    std::unique_ptr<newsflash::task> task_;
    engine::state& state_;

private:
    bool started_;
};


class engine::batch
{
public:

};



engine::engine() : state_(new state)
{
    state_->threads.reset(new threadpool(4));

    state_->threads->on_complete = [&](action* a) 
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->actions.push(a);
        on_async_notify();
    };

    state_->oid = 1;
    state_->bytes_downloaded = 0;
    state_->bytes_queued = 0;
    state_->bytes_written = 0;
    state_->prefer_secure = true;
    state_->started = false;
}

engine::~engine()
{
//    threads_.wait();
}

void engine::set(const account& acc)
{
    auto it = std::find_if(std::begin(state_->accounts), std::end(state_->accounts),
        [&](const account& a) {
            return a.id == acc.id;
        });
    if (it == std::end(state_->accounts))
    {
        state_->accounts.push_back(acc);
        return;
    }

    // const auto& old = *it;

    // const bool general_host_updated = (old.general_host != acc.general_host) ||
    //     (old.general_port != acc.general_port);
    // const bool secure_host_updated = (old.secure_host != acc.secure_host) ||
    //     (old.secure_port != acc.secure_port);
    // const bool credentials_updated = (old.username != acc.username) ||
    //     (old.password != acc.password);

    // for (auto it = conns_.begin(); it != conns_.end(); ++it)
    // {
    //     auto& conn = *it;
    //     if (conn->get_account() != acc.id)
    //         continue;

    //     bool discard = false;

    //     if (general_host_updated)
    //     {
    //         if (conn->hostname() == old.general_host ||
    //             conn->hostport() == old.general_port)
    //             discard = true;
    //     }
    //     if (secure_host_updated)
    //     {
    //         if (conn->hostname() == old.secure_host ||
    //             conn->hostport() == old.secure_port)
    //             discard = true;
    //     }
    //     if (credentials_updated)
    //         discard = true;

    //     if (!discard)
    //         continue;

    //     conn->disconnect();
    //     it = conns_.erase(it);
    // }
}

void engine::download(ui::download spec)
{
    const auto bid = state_->oid;

    for (auto& file : spec.files)
    {
        std::unique_ptr<newsflash::task> job(new class download(
            std::move(file.groups), std::move(file.articles), spec.path, file.name));            

        std::unique_ptr<task> state(new task(spec.account, file.size, file.name, std::move(job), *state_));
        state_->tasks.push_back(std::move(state));
    }

    begin_connect(spec.account);
}

void engine::pump()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock(state_->mutex);
        if (state_->actions.empty())
            return;

        action* a = state_->actions.front();
        state_->actions.pop();
        lock.unlock();

        const auto id = a->get_id();

        bool ready = false;
        for (auto& conn : state_->conns)
        {
            if (conn->id() == id)
            {
                conn->on_action(a);
                ready = true;
            }
        }
        if (ready)
            continue;

        for (auto& task : state_->tasks)
        {
            if (task->id() == id)
            {
                task->on_action(a);
            }
        }
    }
}

void engine::start(std::string logs)
{
    if (state_->start(logs))
    {
        LOG_I("Engine starting");
        LOG_D("Current settings:");
        LOG_D("Overwrite existing files: ", state_->overwrite_existing);
        LOG_D("Discard text content: ", state_->discard_text);
        LOG_D("Prefer secure:", state_->prefer_secure);

        for (const auto& acc : state_->accounts)
        {
            bool has_tasks = false;
            for (const auto& task : state_->tasks)
            {
                if (task->account() == acc.id)
                {
                    has_tasks = true;                
                    break;
                }
            }
            if (!has_tasks)
                continue;

            begin_connect(acc.id);
        }
    }
}

void engine::stop()
{
    // if (!started_)
    //     return;

    // // ...

    // started_ = false;
}

void engine::set_overwrite_existing_files(bool on_off)
{
    state_->overwrite_existing = on_off;
}

void engine::set_discard_text_content(bool on_off)
{
    state_->discard_text = on_off;
}

void engine::set_prefer_secure(bool on_off)
{
    state_->prefer_secure = on_off;
}

void engine::set_throttle(bool on_off)
{
    // todo:
}

void engine::set_throttle_value(unsigned value)
{
    // todo:
}

bool engine::get_overwrite_existing_files() const 
{
    return state_->overwrite_existing;
}

bool engine::get_discard_text_content() const 
{
    return state_->discard_text;
}

bool engine::get_prefer_secure() const 
{
    return state_->prefer_secure;
}

bool engine::get_throttle() const 
{
    // todo:
    return false;
}

bool engine::is_started() const 
{
    return state_->started;
}

void engine::update(std::deque<const ui::task*>& tasklist)
{
    const auto& tasks = state_->tasks;

    tasklist.resize(tasks.size());

    for (std::size_t i=0; i<tasks.size(); ++i)
    {
        tasks[i]->update(tasklist[i]);
    }
}

void engine::update(std::deque<const ui::connection*>& connlist)
{
    const auto& conns = state_->conns;

    connlist.resize(conns.size());

    for (std::size_t i=0; i<conns.size(); ++i)
    {
        conns[i]->update(connlist[i]);
    }
}

void engine::begin_connect(std::size_t account)
{
    if (!state_->started)
        return;

    std::uint16_t num_conns = 0;

    for (auto& conn : state_->conns) 
    {
        if (conn->account() == account)
            num_conns++;
    }

    const auto& acc = state_->find_account(account);

    LOG_D("Account ", acc.name, " has ", num_conns, "/", acc.connections, " connections");

    for (; num_conns < acc.connections; ++num_conns)
    {
        std::unique_ptr<engine::conn> conn(new engine::conn(account, *state_));
        state_->conns.push_back(std::move(conn));
    }
}


} // newsflash
