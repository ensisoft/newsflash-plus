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
#include <list>
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
#include "cmdlist.h"
#include "settings.h"
#include "bodylist.h"
#include "datafile.h"

namespace newsflash
{

#define CASE(x) case x: return #x

const char* str(ui::task::states s)
{
    using state = ui::task::states;
    switch (s)
    {
        CASE(state::queued);
        CASE(state::waiting);
        CASE(state::active);
        CASE(state::paused);
        CASE(state::finalize);
        CASE(state::complete);
        CASE(state::error);
    }
    assert(0);
    return nullptr;    
}

const char* str(ui::connection::states s)
{
    using state = ui::connection::states;
    switch (s)
    {
        CASE(state::disconnected);
        CASE(state::resolving);
        CASE(state::connecting);
        CASE(state::initializing);
        CASE(state::connected);
        CASE(state::active);
        CASE(state::error);
    }    
    assert(0);
    return nullptr;
}

#undef CASE

struct engine::state {
    std::deque<std::unique_ptr<engine::task>> tasks;
    std::deque<std::unique_ptr<engine::conn>> conns;
    std::deque<std::unique_ptr<engine::batch>> batches;
    std::vector<account> accounts;
    std::size_t bytes_downloaded;
    std::size_t bytes_queued;
    std::size_t bytes_ready;
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
    bool group_items;

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

    void submit(action* a, threadpool::thread* thread)
    {
        threads->submit(a, thread);
        num_pending_actions++;

    }
};


class engine::conn
{
private:
    conn(engine::state& state, std::size_t cid)
    {
        std::string file;
        std::string name;
        for (int i=0; i<1000; ++i)
        {
            name = str("connection", i, ".log");
            file = fs::joinpath(state.logpath, name);            
            auto it = std::find_if(std::begin(state.conns), std::end(state.conns),
                [&](const std::unique_ptr<conn>& c) {
                    return c->ui_.logfile == file;
                });
            if (it == std::end(state.conns))
                break;
        }

        ui_.error      = errors::none;
        ui_.state      = states::disconnected;
        ui_.id         = cid;
        ui_.task       = 0;
        ui_.account    = 0;
        ui_.down       = 0;
        ui_.bps        = 0;
        ui_.logfile    = file;        
        ticks_to_ping_ = 30;
        ticks_to_conn_ = 5;
        logger_        = std::make_shared<filelogger>(file, true);
        thread_        = state.threads->allocate();

        LOG_D("Connection ", ui_.id, " log file: ", file);
    }    

    using states = ui::connection::states;
    using errors = ui::connection::errors;

public:

    conn(std::size_t aid, std::size_t cid, engine::state& state) : conn(state, cid)
    {
        const auto& acc = state.find_account(aid);

        connection::spec spec;
        spec.password = acc.password;
        spec.username = acc.username;
        spec.enable_compression = acc.enable_compression;
        spec.enable_pipelining = acc.enable_pipelining;
        if (state.prefer_secure && acc.enable_secure_server)
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
        ui_.account = aid;

        do_action(state, conn_.connect(spec));

        LOG_I("Connection ", ui_.id, " ", ui_.host, ":", ui_.port);        
    }

    conn(std::size_t cid, engine::state& state, const conn& dna) : conn(state, cid)
    {
        ui_.account = dna.ui_.account;
        ui_.host    = dna.ui_.host;
        ui_.port    = dna.ui_.port;
        ui_.secure  = dna.ui_.secure;
        ui_.account = dna.ui_.account;

        const auto& acc = state.find_account(dna.ui_.account);

        connection::spec spec;
        spec.password = acc.password;
        spec.username = acc.username;
        spec.enable_compression = acc.enable_compression;
        spec.enable_pipelining  = acc.enable_pipelining;        
        spec.hostname = dna.ui_.host;
        spec.hostport = dna.ui_.port;
        spec.use_ssl  = dna.ui_.secure;

        do_action(state, conn_.connect(spec));

        LOG_I("Connection ", ui_.id, " ", ui_.host, ":", ui_.port);                
    }

   ~conn()
    {
        LOG_I("Connection ", ui_.id, " deleted");
    }

    void tick(engine::state& state, const std::chrono::milliseconds& elapsed)
    {
        if (ui_.state == states::connected)
        {
            if (--ticks_to_ping_ == 0)
                do_action(state, conn_.ping());

        }
        else if (ui_.state == states::error)
        {
            if (ui_.error == errors::authentication_rejected ||
                ui_.error == errors::no_permission)
                return;

            if (--ticks_to_conn_)
                return;

            LOG_D("Connection ", ui_.id, " reconnecting...");
            ui_.error = errors::none;
            ui_.state = states::disconnected;

            const auto& acc = state.find_account(ui_.account);
            connection::spec s;
            s.password = acc.password;
            s.username = acc.username;
            s.use_ssl  = ui_.secure;
            s.hostname = ui_.host;
            s.hostport = ui_.port;
            s.enable_compression = acc.enable_compression;
            s.enable_pipelining  = acc.enable_pipelining;
            do_action(state, conn_.connect(s));                
        }
    }

    void stop(engine::state& state)
    {
        if (ui_.state == states::disconnected)
            return;

        conn_.cancel();
        if (ui_.state == states::connected || 
            ui_.state == states::active)
        {
            do_action(state, conn_.disconnect());
        }

        state.threads->detach(thread_);
    }

    void execute(engine::state& state, std::shared_ptr<cmdlist> cmds, std::size_t tid, std::string desc)
    {
        ui_.task  = tid;
        ui_.desc  = std::move(desc);
        ui_.state = states::active;
        do_action(state, conn_.execute(cmds, tid));
    }

    void update(ui::connection& ui)
    {
        ui = ui_;
        ui.bps = 0;
        if (ui_.state == states::active)
        {
            // if bytes value hasn't increased since the last read
            // then bps reading will not be valid but the connection has stalled.
            // note that ui
            const auto bytes = ui_.down;
            ui_.down = conn_.bytes();
            if (bytes != ui_.down)
                ui.bps = conn_.bps();
        }
    }
    void on_action(engine::state& state, action* a)
    {
        std::unique_ptr<action> act(a);

        ticks_to_ping_ = 30;
        ticks_to_conn_ = 5;

        if (act->has_exception())
        {
            ui_.state = states::error;
            ui_.bps   = 0;
            ui_.task  = 0;
            ui_.desc  = "";
            ui::error err;
            err.resource = ui_.host;

            try
            {
                act->rethrow();
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
                    case connection::error::pipeline_reset:
                        ui_.error = ui::connection::errors::other;
                        break;
                }
                err.what = e.what();
            }
            catch (const std::system_error& e)
            {
                const auto code = e.code();
                if (code == std::errc::connection_refused)
                    ui_.error = ui::connection::errors::refused;
                else ui_.error = ui::connection::errors::other;

                err.code = code;
                err.what = e.what();
            }
            catch (const std::exception &e)
            {
                ui_.error = errors::other;                
                err.what  = e.what();
            }

            if (err.code) {
                LOG_E("Connection ", ui_.id, " (", err.code.value(), ") ", err.code.message());
            } else {
                LOG_E("Connection ", ui_.id, " ", err.what);
            }

            state.on_error_callback(err);
            state.logger->flush();
            logger_->flush();
            return;
        }

        auto next = conn_.complete(std::move(act));
        if (next)
            do_action(state, std::move(next));
        else if (ui_.state == states::initializing)
            ui_.state = states::connected;
        else if (ui_.state == states::active)
        {
           ui_.state = states::connected;
           ui_.bps   = 0;
           ui_.task  = 0;
           ui_.desc  = "";
           LOG_D("Connection ", ui_.id, " completed execute!");           
           LOG_D("Connection ", ui_.id, " => state::connected");
        }
        logger_->flush();
    }
    bool is_ready() const 
    {
        return ui_.state == states::connected;
    }

    std::size_t task() const 
    { return ui_.task; }

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
    void do_action(engine::state& state, std::unique_ptr<action> a)
    {
        auto* ptr = a.get();

        if (dynamic_cast<class connection::resolve*>(ptr))
            ui_.state = states::resolving;
        else if (dynamic_cast<class connection::connect*>(ptr))
            ui_.state = states::connecting;
        else if (dynamic_cast<class connection::initialize*>(ptr))
            ui_.state = states::initializing;
        else if (dynamic_cast<class connection::execute*>(ptr))
            ui_.state = states::active;
        else if (dynamic_cast<class connection::disconnect*>(ptr))
            ui_.state = states::disconnected;

        LOG_D("Connection ", ui_.id,  " => ", str(ui_.state));
        LOG_D("Connection ", ui_.id, " current task ", ui_.task, " (", ui_.desc, ")");

        a->set_id(ui_.id);
        a->set_log(logger_);
        state.submit(a.release(), thread_);
    }

private:
    ui::connection ui_;
    connection conn_;
    std::shared_ptr<logger> logger_;
    threadpool::thread* thread_;
    unsigned ticks_to_ping_;
    unsigned ticks_to_conn_;
};



class engine::task 
{
public:
    using states = ui::task::states;
    using clock  = std::chrono::steady_clock;

    task(std::size_t aid, std::size_t tid, std::size_t bid, std::uint64_t size, 
        std::string desc, std::unique_ptr<newsflash::task> task) : task_(std::move(task))
    {
        ui_.state      = states::queued;
        ui_.id         = tid;
        ui_.bid        = bid;
        ui_.account    = aid;
        ui_.desc       = desc;
        ui_.size       = size;
        ui_.runtime    = 0;
        ui_.etatime    = 0;
        ui_.completion = 0.0;
        started_       = false;
        executed_      = false;
        num_pending_actions_ = 0;

        LOG_I("Task ", ui_.id, " (", ui_.desc, ") created");
    }

   ~task()
    {
        LOG_I("Task ", ui_.id, " deleted");
    }

    void kill(engine::state& state)
    {
        if (ui_.state != states::complete)
        {
            for (auto& cmd : cmds_)
                cmd->cancel();

            if (task_)
                do_action(state, task_->kill());

            state.bytes_queued -= ui_.size;
        }
    }

    void configure(const settings& s)
    {
        if (task_)
            task_->configure(s);
    }

    void tick(engine::state& state, const std::chrono::milliseconds& elapsed)
    {
        // todo: actually measure the time
        if (ui_.state == states::active ||
            ui_.state == states::finalize)
            ui_.runtime++;
    }

    bool eligible_for_run() const 
    {
        if (ui_.state == states::waiting ||
            ui_.state == states::queued ||
            ui_.state == states::active)
            return true;
        
        return false;
    }

    bool run(engine::state& state, engine::conn& conn)
    {
        if (ui_.state == states::complete || 
            ui_.state == states::error ||
            ui_.state == states::paused ||
            ui_.state == states::finalize)
            return false;

        auto cmds = task_->create_commands();
        if (!cmds)
        {
            executed_ = true;
            update_completion(state);
            return false;
        }

        std::shared_ptr<cmdlist> cmd(cmds.release());
        cmds_.push_back(cmd);

        conn.execute(state, cmd, ui_.id, ui_.desc);

        goto_state(state, states::active);
        return true;
    }

    void complete(engine::state& state, std::shared_ptr<cmdlist> cmds)
    {
        cmds_.erase(std::find(std::begin(cmds_), 
            std::end(cmds_), cmds));

        if (ui_.state == states::error)
            return;

        // code to dump raw NNTP data to the disk
        // auto& list = dynamic_cast<bodylist&>(*cmds.get());
        // const auto& contents = list.get_buffers();
        // const auto& articles = list.get_messages();
        // for (int i=0; i<contents.size(); ++i)
        // {
        //     const auto& content = contents[i];
        //     const auto& article = articles[i];
        //     const auto filename = fs::joinpath("/tmp/Newsflash/", article);
        //     std::ofstream out(filename, std::ios::binary);
        //     out.write(content.content(),
        //         content.content_length());
        //     out.close();
        // }

        std::vector<std::unique_ptr<action>> actions;

        task_->complete(*cmds, actions);
        for (auto& a : actions)
            do_action(state, std::move(a));

        update_completion(state);
    }

    void pause(engine::state& state)
    {
        if (ui_.state == states::paused ||
            ui_.state == states::error ||
            ui_.state == states::complete)
            return;

        for (auto& cmd : cmds_)
            cmd->cancel();

        goto_state(state, states::paused);
    }

    void resume(engine::state& state)
    {
        if (ui_.state != states::paused)
            return;

        if (started_)
            goto_state(state, states::waiting);
        else goto_state(state, states::queued);
    }

    void update(ui::task& ui)
    {
        ui = ui_;
    }

    void on_action(engine::state& state, action* a)
    {
        std::unique_ptr<action> act(a);

        num_pending_actions_--;

        if (ui_.state == states::error)
            return;

        ui::error err;
        err.resource = ui_.desc;
        try
        {
            if (act->has_exception())
                act->rethrow();

            std::vector<std::unique_ptr<action>> actions;

            task_->complete(*act, actions);
            act.reset();

            for (auto& a : actions)
                do_action(state, std::move(a));

            update_completion(state);
            return;
        }
        catch (const std::system_error& e)
        {
            err.code = e.code();
            err.what = e.what();
        }
        catch (const std::exception& e)
        {
            err.what = e.what();
        }

        if(err.code) {
            LOG_E("Task ", ui_.id, " (", err.code.value(), ") ", err.code.message());
        } else {
            LOG_E("Task ", ui_.id, " ", err.what);
        }

        state.on_error_callback(err);
        state.logger->flush();
        goto_state(state, states::error);            
    }

    std::size_t account() const 
    { return ui_.account; }

    std::size_t tid() const
    { return ui_.id; }

    std::size_t bid() const 
    { return ui_.bid; }

private:
    void update_completion(engine::state& state)
    {
        ui_.completion = task_->completion();

        if (num_pending_actions_)
            return;
        if (!cmds_.empty())
            return;

        if (executed_)
            goto_state(state, states::finalize);
        else if (ui_.state == states::active)
            goto_state(state, states::waiting);
        else if (ui_.state == states::waiting)
            goto_state(state, states::waiting);
        else if (ui_.state == states::finalize)
            goto_state(state, states::complete);
    }

    void do_action(engine::state& state, std::unique_ptr<action> a) 
    {
        if (!a) return;

        a->set_id(ui_.id);

        state.submit(a.release());

        num_pending_actions_++;
    }

    void goto_state(engine::state& state, states new_task_state)
    {
        ui_.state = new_task_state;
        switch (new_task_state)
        {
            case states::active:
                if (!started_)
                {
                    LOG_D("Task ", ui_.id,  " starting...");
                    do_action(state, task_->start());
                    started_ = true;
                }
                LOG_D("Task ", ui_.id, " has ", cmds_.size(),  " command lists");
                LOG_D("Task ", ui_.id, " has ", num_pending_actions_, " pending actions");
                break;

            case states::error:
                ui_.etatime = 0;
                //state_.bytes_ready += ui_.size; 
                do_action(state, task_->kill());
                task_.release();
                break;

            case states::finalize:
                {
                    ui_.etatime = 0;
                    auto act = task_->finalize();
                    if (!act)
                    {
                        ui_.state = states::complete;
                        ui_.completion = 100.0f;
                        state.bytes_ready += ui_.size;
                    }
                    else do_action(state, std::move(act));
                }
                break;

            case states::complete:
                ui_.completion = 100.0;
                state.bytes_ready += ui_.size;
                break;

            default:
                break;
        }

        LOG_D("Task ", ui_.id, " => ", str(new_task_state));
    }

private:
    ui::task ui_;    
    std::unique_ptr<newsflash::task> task_;
    std::list<std::shared_ptr<cmdlist>> cmds_;
    std::size_t num_pending_actions_;
private:
    bool started_;
    bool executed_;
};


class engine::batch
{
public:
    using states = ui::task::states;

    batch(std::size_t account, std::size_t id, std::uint64_t size, std::size_t num_tasks, std::string desc)
    {
        ui_.state   = states::queued;
        ui_.id      = id;
        ui_.bid     = id;
        ui_.account = account;
        ui_.desc    = std::move(desc);
        ui_.size    = size;
        ui_.runtime = 0;
        ui_.etatime = 0;
        ui_.completion = 0.0;

        LOG_I("Batch ", ui_.id, " (", ui_.desc, ") created");
    }
   ~batch()
    {
        LOG_I("Batch ", ui_.id, " deleted");
    }

    void pause(engine::state& state)
    {
        for (auto& task : state.tasks)
        {
            if (task->bid() == ui_.id)
                task->pause(state);
        }
    }

    void resume(engine::state& state)
    {
        for (auto& task : state.tasks)
        {
            if (task->bid() == ui_.id)
                task->resume(state);
        }
    }

    void update(ui::task& ui)
    {
        ui = ui_;
    }

    void kill(engine::state& state)
    {
        // partition tasks so that the tasks that belong to this batch
        // are at the end of the the task list.
        auto it = std::stable_partition(std::begin(state.tasks), std::end(state.tasks),
            [&](const std::unique_ptr<task>& t) {
                return t->bid() != ui_.id;
            });

        for (auto i=it; i != std::end(state.tasks); ++i)
            (*i)->kill(state);

        state.tasks.erase(it, std::end(state.tasks));
    }

    void update(engine::state& state, const task& t)
    {

    }

private:
    ui::task ui_;
private:
    std::size_t num_tasks_;
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

    state_->oid                 = 1;
    state_->bytes_downloaded    = 0;
    state_->bytes_queued        = 0;
    state_->bytes_ready         = 0;
    state_->bytes_written       = 0;
    state_->num_pending_actions = 0;
    state_->prefer_secure       = true;
    state_->started             = false;
    state_->group_items         = false;
}

engine::~engine()
{
    assert(state_->num_pending_actions == 0);
    assert(state_->conns.empty());
    assert(state_->started == false);

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
        {
            const auto cid = state_->oid++;
            state_->conns.emplace_back(new engine::conn(acc.id, cid, *state_));
        }
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
    const auto bid = state_->oid++;
    const auto aid = spec.account;

    settings s;
    s.discard_text_content = state_->discard_text;
    s.overwrite_existing_files = state_->overwrite_existing;

    std::uint64_t batch_size = 0;

    for (auto& file : spec.files)
    {
        LOG_D("New file download created");
        LOG_D("Download has ", file.articles.size(), " articles");
        LOG_D("Download is stored in '", spec.path, "'");

        const auto tid = state_->oid++;

        std::unique_ptr<newsflash::download> job(new class download(
            std::move(file.groups), std::move(file.articles), spec.path, file.name));            
        job->configure(s);

        std::unique_ptr<task> state(new task(aid, tid, bid, file.size, file.name, std::move(job)));
        state_->tasks.push_back(std::move(state));
        state_->bytes_queued += file.size;

        batch_size += file.size;
    }

    const auto num_tasks = spec.files.size();

    std::unique_ptr<batch> batch(new class batch(aid, bid, batch_size, num_tasks, spec.desc));
    state_->batches.push_back(std::move(batch));

    connect();
}

bool engine::pump()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock(state_->mutex);
        if (state_->actions.empty())
            break;

        //LOG_D("Dispatch action");

        action* a = state_->actions.front();
        state_->actions.pop();
        lock.unlock();

        bool action_dispatched = false;

        if (auto* e = dynamic_cast<class connection::execute*>(a))
        {
            auto cmds  = e->get_cmdlist();
            auto tid   = e->get_tid();
            auto bytes = e->get_bytes_transferred();
            auto tit  = std::find_if(std::begin(state_->tasks), std::end(state_->tasks),
                [&](const std::unique_ptr<engine::task>& t) {
                    return t->tid() == tid;
                });
            if (tit != std::end(state_->tasks))
                (*tit)->complete(*state_, cmds);

            state_->bytes_downloaded += bytes;
        }
        else if (auto* w = dynamic_cast<class datafile::write*>(a))
        {
            auto bytes = w->get_write_size();
            if (!w->has_exception())
                state_->bytes_written += bytes;
        }

        const auto id = a->get_id();

        for (auto& conn : state_->conns)
        {
            if (conn->id() != id)
                continue;

            conn->on_action(*state_, a);
            if (conn->is_ready())
            {
                for (auto& task : state_->tasks)
                {
                    if (task->account() != conn->account())
                        continue;
                    if (task->run(*state_, *conn))
                        break;
                }
            }
            action_dispatched = true;
            break;
        }
        if (!action_dispatched)
        {
            for (auto& task : state_->tasks)
            {
                if (task->tid() == id)
                {
                    task->on_action(*state_, a);
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
    for (auto& conn : state_->conns)
    {
    // todo: measure time here instead of relying on the clientr        
        conn->tick(*state_, std::chrono::seconds(1));
    }

    for (auto& task : state_->tasks)
    {
        task->tick(*state_, std::chrono::seconds(1));
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
        conn->stop(*state_);
    }
    state_->conns.clear();
    state_->started = false;
    state_->logger->flush();
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

    settings s;
    s.overwrite_existing_files = on_off;
    s.discard_text_content = state_->discard_text;

    for (auto& t: state_->tasks)
        t->configure(s);
}

void engine::set_discard_text_content(bool on_off)
{
    state_->discard_text = on_off;

    settings s;
    s.discard_text_content = on_off;
    s.overwrite_existing_files = state_->overwrite_existing;

    for (auto& t : state_->tasks)
        t->configure(s);
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

unsigned engine::get_throttle_value() const
{ 
    // todo:
    return 0;
}

std::uint64_t engine::get_bytes_queued() const 
{
    return state_->bytes_queued;
}

std::uint64_t engine::get_bytes_ready() const 
{
    return state_->bytes_ready;
}

std::uint64_t engine::get_bytes_written() const
{
    return state_->bytes_written;
}

std::uint64_t engine::get_bytes_downloaded() const
{
    return state_->bytes_downloaded;
}

std::string engine::get_logfile() const 
{
    return fs::joinpath(state_->logpath, "engine.log");
}

void engine::set_group_items(bool on_off)
{
    state_->group_items = on_off;
}

bool engine::get_group_items() const 
{
    return state_->group_items;
}

bool engine::is_started() const 
{
    return state_->started;
}

void engine::update(std::deque<ui::task>& tasklist)
{
    if (state_->group_items)
    {
        const auto& tasks = state_->batches;
        tasklist.resize(tasks.size());
        for (std::size_t i=0; i<tasks.size(); ++i)
            tasks[i]->update(tasklist[i]);
    }
    else
    {
        const auto& tasks = state_->tasks;
        tasklist.resize(tasks.size());
        for (std::size_t i=0; i<tasks.size(); ++i)
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
    (*it)->stop(*state_);
    state_->conns.erase(it);
}

void engine::clone_connection(std::size_t i)
{
    LOG_D("Clone connection ", i);

    assert(i < state_->conns.size());

    const auto cid = state_->oid++;

    auto& dna  = state_->conns[i];
    auto dolly = std::unique_ptr<conn>(new conn(cid, *state_, *dna));
    state_->conns.push_back(std::move(dolly));
}

void engine::kill_task(std::size_t i)
{
    if (state_->group_items)
    {
        LOG_D("Kill batch ", i);

        assert(i < state_->batches.size());

        auto it = state_->batches.begin();
        it += i;
        (*it)->kill(*state_);
        state_->batches.erase(it);
    }
    else
    {
        LOG_D("Kill task ", i);

        assert(i < state_->tasks.size());

        auto it = state_->tasks.begin();
        it += i;
        (*it)->kill(*state_);        
        state_->tasks.erase(it);

    }

    if (state_->tasks.empty())
    {
        state_->bytes_queued = 0;
        state_->bytes_ready  = 0;
    }
}

void engine::pause_task(std::size_t index)
{
    if (state_->group_items)
    {
        LOG_D("Pause batch ", index);

        assert(index < state_->batches.size());

        state_->batches[index]->pause(*state_);
    }
    else
    {
        LOG_D("Pause task ", index);

        assert(index < state_->tasks.size());

        state_->tasks[index]->pause(*state_);
    }
}

void engine::resume_task(std::size_t index)
{
    if (state_->group_items)
    {
        LOG_D("Resume batch ", index);

        assert(index < state_->batches.size());

        state_->batches[index]->resume(*state_);
    }
    else
    {
        LOG_D("Resume task ", index);

        assert(index < state_->tasks.size());

        state_->tasks[index]->resume(*state_);

        connect();
    }
}

void engine::move_task_up(std::size_t index)
{
    if (state_->group_items)
    {

    }
    else
    {
        assert(state_->tasks.size() > 1);        
        assert(index < state_->tasks.size());
        assert(index > 0);
        std::swap(state_->tasks[index], state_->tasks[index - 1]);
    }
}

void engine::move_task_down(std::size_t index)
{
    if (state_->group_items)
    {

    }
    else
    {
        assert(state_->tasks.size() > 1);
        assert(index < state_->tasks.size()-1);
        std::swap(state_->tasks[index], state_->tasks[index + 1]);
    }
}

void engine::connect()
{
    if (!state_->started)
        return;

    // scan the list of queued tasks and look for one that is ready to be run.
    // when the task is found see which account it belongs to and spawn the connections
    // finally if we have already ready connections we set them to work on the 
    // eligible task possibly looking for a new task once the current task
    // has no more work to be done.

    for (auto tit = std::begin(state_->tasks); tit != std::end(state_->tasks); ++tit)
    {
        auto& task = *tit;
        if (!task->eligible_for_run())
            continue;

        // how many conns do we have in total associated with this account.
        const auto num_conns = std::count_if(std::begin(state_->conns), std::end(state_->conns),
            [&](const std::unique_ptr<engine::conn>& c) {
                return c->account() == task->account();
            });

        // must find the account
        const auto a = std::find_if(std::begin(state_->accounts), std::end(state_->accounts),
            [&](const account& a) {
                return a.id == task->account();
            });
        ASSERT(a != std::end(state_->accounts));

        // if there's space, spawn new connections.
        for (auto i=num_conns; i<(*a).connections; ++i)
        {
            const auto cid = state_->oid++;
            state_->conns.emplace_back(new engine::conn((*a).id, cid, *state_));
        }

        // starting with the current task assing ready connections to work on this task.
        engine::task* candidate = task.get();

        for (auto& conn : state_->conns)
        {
            if (!conn->is_ready())
                continue;

            // if candidate returns true it means it might have more work
            // on next iteration as well. 
            if (candidate->run(*state_, *conn))
                continue;

            // candidate didn't have any more work. hence we can inspect the  
            // remainder of the task list looking for another task that belongs
            // to the *same* account and is runnable
            auto it = std::find_if(tit, std::end(state_->tasks),
                [&](const std::unique_ptr<engine::task>& t) {
                    return t->account() == conn->account() && t->eligible_for_run();
                });
            if (it == std::end(state_->tasks))
                break;
            candidate = (*it).get();
        }
    }
}


} // newsflash
