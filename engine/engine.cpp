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
#include "socket.h"

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

    std::unique_ptr<class logger> logger;

    std::mutex  mutex;
    std::queue<action*> actions;
    std::unique_ptr<threadpool> threads;
    std::size_t num_pending_actions;

    bool prefer_secure;
    bool overwrite_existing;
    bool discard_text;
    bool started;

    engine::on_error on_error_callback;
    engine::on_file  on_file_callback;
    engine::on_async_notify on_notify_callback;

   ~state()
    {
        batches.clear();
        tasks.clear();
        conns.clear();

    }
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

        if (logs != logpath)
        {
            const auto logfile = fs::joinpath(logs, "engine.log");

            std::unique_ptr<class logger> log(new filelogger(logfile, true));
            if (log->is_open())
                set_thread_log(log.get());
            else set_thread_log(nullptr);

            logger = std::move(log);            
        }

        logpath = logs;
        started = true;
        return true;
    }

    void submit(action* a)
    {
        threads->submit(a);
        num_pending_actions++;
    }
};


class engine::conn
{
private:
    conn(engine::state& s) : state_(s)
    {
        ui_.error   = error::none;
        ui_.state   = state::disconnected;
        ui_.id      = s.oid++;
        ui_.task    = 0;
        ui_.account = 0;
        ui_.down    = 0;
        ui_.bps     = 0;
        ticks_to_ping_ = 30;
        ticks_to_conn_ = 5;
        conn_number_   = ++current_num_connections;
        conn_logger_   = std::make_shared<filelogger>(fs::joinpath(s.logpath,
            str("connection", conn_number_, ".log")), true);
    }    

    using state = ui::connection::states;
    using error = ui::connection::errors;

public:

    conn(std::size_t account, engine::state& s) : conn(s)
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

        do_action(conn_.connect(spec));

        LOG_I("Connection ", conn_number_, " ", ui_.host, ":", ui_.port);        
    }

    conn(const conn& other) : conn(other.state_)
    {
        ui_.account = other.ui_.account;
        ui_.host    = other.ui_.host;
        ui_.port    = other.ui_.port;
        ui_.secure  = other.ui_.secure;

        const auto& acc = state_.find_account(ui_.account);

        connection::spec spec;
        spec.password = acc.password;
        spec.username = acc.username;
        spec.hostname = other.ui_.host;
        spec.hostport = other.ui_.port;
        spec.use_ssl  = other.ui_.secure;

        do_action(conn_.connect(spec));

        LOG_I("Connection ", conn_number_, " ", ui_.host, ":", ui_.port);                
    }

   ~conn()
    {
        stop();

        LOG_I("Deleted connection ", conn_number_);

        --current_num_connections;
    }

    void tick()
    {
        if (ui_.state == state::connected)
        {
            if (--ticks_to_ping_ == 0)
                do_action(conn_.ping());

        }
        else if (ui_.state == state::error)
        {
            if (ui_.error == error::resolve ||
                ui_.error == error::refused ||
                ui_.error == error::network ||
                ui_.error == error::timeout)
            {
                if (--ticks_to_conn_)
                    return;

                LOG_D("Reconnecting...");
                ui_.error = error::none;
                ui_.state = state::disconnected;

                const auto& acc = state_.find_account(ui_.account);
                connection::spec s;
                s.password = acc.password;
                s.username = acc.username;
                s.use_ssl  = ui_.secure;
                s.hostname = ui_.host;
                s.hostport = ui_.port;
                do_action(conn_.connect(s));                
            }
        }
    }

    void stop()
    {
        if (ui_.state == state::disconnected)
            return;

        conn_.cancel();
        if (ui_.state == state::connected || 
            ui_.state == state::active)
        {
            do_action(conn_.disconnect());
        }
    }

    void update(ui::connection& ui)
    {
        ui = ui_;
    }
    void on_action(action* a)
    {
        ticks_to_ping_ = 30;
        ticks_to_conn_ = 5;

        if (a->has_exception())
        {
            ui_.state = state::error;
            ui_.bps   = 0;
            ui_.task  = 0;
            ui_.desc  = "";
            ui::error err;
            err.resource = ui_.host;

            try
            {
                a->rethrow();
            }
            catch (const connection::exception& e)
            {
                switch (e.error())
                {
                    case connection::error::resolve:
                        ui_.error = ui::connection::errors::resolve;
                        break;
                    case connection::error::authentication_rejected:
                        ui_.error = ui::connection::errors::authentication_rejected;
                        break;
                    case connection::error::no_permission:
                        ui_.error = ui::connection::errors::no_permission;
                        break;
                    case connection::error::network:
                        ui_.error = ui::connection::errors::network;
                        break;
                    case connection::error::timeout:
                        ui_.error = ui::connection::errors::timeout;
                        break;
                }
                LOG_E("Connection ", conn_number_, " ", e.what());
                err.resource = ui_.host;
                err.what     = e.what();
            }
            catch (const std::system_error& e)
            {
                const auto code = e.code();
                if (code == std::errc::connection_refused)
                    ui_.error = ui::connection::errors::refused;
                else ui_.error = ui::connection::errors::other;

                LOG_E("Connection ", conn_number_, " (", code.value(), ") ", code.message());
                err.code = code;
                err.what = e.what();
            }
            catch (const std::exception &e)
            {
                ui_.error = error::other;                
                LOG_E("Connection ", conn_number_, " ", e.what());
                err.what = e.what();
            }

            state_.on_error_callback(err);

            conn_logger_->flush();
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

        conn_logger_->flush();
    }

    std::size_t account() const
    { return ui_.account; }

    std::size_t id() const 
    { return ui_.id; }

    std::uint16_t port() const 
    { return ui_.port; }

    const std::string& host() const 
    { return ui_.host; }

    const std::string& username() const
    { return conn_.username(); }

    const std::string& password() const 
    { return conn_.password(); }

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
        a->set_log(conn_logger_);
        a->set_affinity(action::affinity::any_thread);
        state_.submit(a.release());
    }

private:
    ui::connection ui_;
    connection conn_;
    engine::state& state_;
    std::shared_ptr<logger> conn_logger_;
    unsigned conn_number_;
    unsigned ticks_to_ping_;
    unsigned ticks_to_conn_;
private:
    static unsigned current_num_connections;
};

unsigned engine::conn::current_num_connections;

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

    void update(ui::task& ui)
    {
        ui = ui_;
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
        state_->on_notify_callback();
    };

    state_->oid = 1;
    state_->bytes_downloaded = 0;
    state_->bytes_queued = 0;
    state_->bytes_written = 0;
    state_->num_pending_actions = 0;
    state_->prefer_secure = true;
    state_->started = false;
}

engine::~engine()
{
    state_->threads->shutdown();
    state_->threads.reset();
}

void engine::set_account(const account& acc)
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

    const auto& old = *it;

    // if the hosts or credentials are modified restart the connections.
    const auto secure_modified = old.secure_host != acc.secure_host ||
        old.secure_port != acc.secure_port;
    const auto general_modified = old.general_host != acc.general_host ||
        old.general_port != acc.general_port;
    const auto credentials_modified = old.username != acc.username ||
        old.password != acc.password;

    *it = acc;

    // remove connections whose matching properties have been modified.
    auto end = std::remove_if(std::begin(state_->conns), std::end(state_->conns),
        [&](const std::unique_ptr<conn>& c) 
        {
            if (c->account() != acc.id)
                return false;

            const auto& host = c->host();
            const auto& port = c->port();

            if (secure_modified)
            {
                if (host != acc.secure_host || port != acc.secure_port)
                    return true;
            }
            else if (general_modified)
            {
                if (host != acc.general_host || port != acc.general_port)
                    return true;
            }
            else if (credentials_modified)
            {
                const auto& user = c->username();
                const auto& pass = c->password();
                if (user != acc.username || pass != acc.password)
                    return true;
            }
            return false;
        });

    state_->conns.erase(end, std::end(state_->conns));

    //const auto have_tasks = std::find_if(std::begin(state_->tasks), std::end(state_->tasks),
    //    [&](const std::unique_ptr<task>& t) {
    //        return t->account() == acc.id;
    //    }) != std::end(state_->tasks);
    //if (!have_tasks)
    //    return;

    // count the number of connections we have now for this account.
    // if there are less than the minimum number of allowed, we spawn
    // new connections, otherwise the shrink the connection list down
    // to the max allowed.
    const auto num_conns = std::count_if(std::begin(state_->conns), std::end(state_->conns),
        [&](const std::unique_ptr<conn>& c) {
            return c->account() == acc.id;
        });

    if (num_conns < acc.connections)
    {
        if (!state_->started)
            return;

        for (auto i = num_conns; i<acc.connections; ++i)
            state_->conns.emplace_back(new engine::conn(acc.id, *state_));
    }
    else if (num_conns > acc.connections)
    {
        state_->conns.resize(acc.connections);
    }
}

void engine::del_account(std::size_t id)
{
    auto end = std::remove_if(std::begin(state_->conns), std::end(state_->conns),
        [&](const std::unique_ptr<conn>& c) {
            return c->account() == id;
        });

    state_->conns.erase(end, std::end(state_->conns));

    auto it = std::find_if(std::begin(state_->accounts), std::end(state_->accounts),
        [&](const account& a) { 
            return a.id == id;
        });
    ASSERT(it != std::end(state_->accounts));

    state_->accounts.erase(it);
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

    connect();
}

bool engine::pump()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock(state_->mutex);
        if (state_->actions.empty())
            break;

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
        if (!ready)
        {
            for (auto& task : state_->tasks)
            {
                if (task->id() == id)
                {
                    task->on_action(a);
                }
            }
        }
        state_->num_pending_actions--;
        state_->logger->flush();        
    }

    return state_->num_pending_actions != 0;
}

void engine::tick()
{
    // todo: measure time here instead of relying on the clientr

    for (auto& conn : state_->conns)
    {
        conn->tick();
    }

    connect();
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

        connect();
    }
}

void engine::stop()
{
    if (!state_->started)
        return;

    LOG_I("Engine stopping");
    for (auto& conn : state_->conns)
    {
        conn->stop();
    }
    state_->conns.clear();
    state_->started = false;
}

void engine::set_error_callback(on_error error_callback)
{
    state_->on_error_callback = std::move(error_callback);
}

void engine::set_file_callback(on_file file_callback)
{
    state_->on_file_callback = std::move(file_callback);
}

void engine::set_notify_callback(on_async_notify notify_callback)
{
    state_->on_notify_callback = std::move(notify_callback);
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

void engine::update(std::deque<ui::task>& tasklist)
{
    const auto& tasks = state_->tasks;

    tasklist.resize(tasks.size());

    for (std::size_t i=0; i<tasks.size(); ++i)
    {
        tasks[i]->update(tasklist[i]);
    }
}

void engine::update(std::deque<ui::connection>& connlist)
{
    const auto& conns = state_->conns;

    connlist.resize(conns.size());

    for (std::size_t i=0; i<conns.size(); ++i)
    {
        conns[i]->update(connlist[i]);
    }
}

void engine::kill_connection(std::size_t i)
{
    LOG_D("Kill connection ", i);

    assert(i < state_->conns.size());

    auto it = state_->conns.begin();
    it += i;
    state_->conns.erase(it);
}

void engine::clone_connection(std::size_t i)
{
    LOG_D("Clone connection ", i);

    assert(i < state_->conns.size());

    auto& dna  = state_->conns[i];
    auto dolly = std::unique_ptr<conn>(new conn(*dna));
    state_->conns.push_back(std::move(dolly));
}


void engine::connect()
{
    if (!state_->started)
        return;

    for (const auto& acc : state_->accounts)
    {
        const auto num_conns = std::count_if(std::begin(state_->conns), std::end(state_->conns),
            [&](const std::unique_ptr<conn>& c) {
                return c->account() == acc.id;
            });
        
        for (auto i = num_conns; i<acc.connections; ++i)
            state_->conns.emplace_back(new engine::conn(acc.id, *state_));
    }
}


} // newsflash
