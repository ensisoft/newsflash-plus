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
    std::list<std::shared_ptr<cmdlist>> cmds;
    std::size_t bytes_downloaded;
    std::size_t bytes_queued;
    std::size_t bytes_ready;
    std::size_t bytes_written;
    std::size_t oid; // object id for tasks/connections
    std::size_t fill_account;
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
    bool repartition_task_list;


    engine::on_error on_error_callback;
    engine::on_file  on_file_callback;
    engine::on_batch on_batch_callback;
    engine::on_async_notify on_notify_callback;

   ~state()
    {
        tasks.clear();
        conns.clear();
        batches.clear();
        set_thread_log(nullptr);
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

    action* get_action() 
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (actions.empty())
            return nullptr;
        action* act = actions.front(); actions.pop();
        return act;

    }

    engine::batch& find_batch(std::size_t id);

    void execute();
    void enqueue(const task& t, std::shared_ptr<cmdlist> cmd);
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
            // if the bytes value hasn't increased since the last
            // read then the bps value cannot be valid but has become
            // stale since the connection has not ready any data. (i.e. it's stalled)
            const auto bytes_before  = ui_.down;
            const auto bytes_current = conn_.bytes();
            if (bytes_current != bytes_before)
            {
                ui_.bps  = ui.bps  = conn_.bps();
                ui_.down = ui.down = bytes_current;
            }
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

            LOG_E("Connection ", ui_.id, " (", err.code.value(), ") ", err.code.message());
            LOG_E("Connection ", ui_.id, " ", err.what);

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

    double bps() const
    {
        if (ui_.state == states::active)
            return ui_.bps;
        return 0.0;
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

    struct transition {
        states previous;
        states current;

        operator bool() const 
        {
            return previous != current;
        }
    };

    constexpr static auto no_transition = transition{states::queued, states::queued};

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
        num_active_cmdlists_ = 0;
        num_active_actions_  = 0;
        num_complete_bytes_  = size;

        LOG_I("Task ", ui_.id, " (", ui_.desc, ") created");
    }

   ~task()
    {
        LOG_I("Task ", ui_.id, " deleted");
    }

    void kill(engine::state& state)
    {
        LOG_D("Task ", ui_.id, " kill");

        if (ui_.state != states::complete)
        {
            auto it = std::stable_partition(std::begin(state.cmds), std::end(state.cmds),
                [&](const std::shared_ptr<cmdlist>& cmd) {
                    return cmd->task() != ui_.id;
                });

            for (auto i=it; i != std::end(state.cmds); ++i)
                (*i)->cancel();

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
        {
            return task_->has_commands();
        }
        return false;
    }

    transition run(engine::state& state)
    {
        if (ui_.state == states::complete || 
            ui_.state == states::error ||
            ui_.state == states::paused ||
            ui_.state == states::finalize)
            return no_transition;

        std::unique_ptr<cmdlist> unique(task_->create_commands());
        std::shared_ptr<cmdlist> shared(unique.release());
        shared->set_account(ui_.account);
        shared->set_task(ui_.id);
        state.enqueue(*this, shared);
        ++num_active_cmdlists_;

        return goto_state(state, states::active);
    }

    transition complete(engine::state& state, std::shared_ptr<cmdlist> cmds)
    {
        --num_active_cmdlists_;

        if (ui_.state == states::error)
            return no_transition;

        if (!cmds->is_good())
        {
            if (state.fill_account && 
                state.fill_account != cmds->account())
            {
                cmds->set_account(state.fill_account);
                state.enqueue(*this, cmds);
                ++num_active_cmdlists_;
                return no_transition;
            }
            else
            {
                ui_.error.set(ui::task::errors::unavailable);
                return no_transition;
            }
        }
        else
        {
            const auto& buffers = cmds->get_buffers();
            for (const auto& buff : buffers)
            {
                const auto status = buff.content_status();
                if (status == buffer::status::success)
                    continue;

                if (state.fill_account && 
                    state.fill_account != cmds->account())
                {
                    cmds->set_account(state.fill_account);
                    state.enqueue(*this, cmds);
                    ++num_active_cmdlists_;
                    return no_transition;
                }
                else if (status == buffer::status::unavailable)
                    ui_.error.set(ui::task::errors::unavailable);
                else if (status == buffer::status::dmca)
                    ui_.error.set(ui::task::errors::dmca);
                else if (status == buffer::status::error)
                    return goto_state(state, states::error);
            }
        }

        std::vector<std::unique_ptr<action>> actions;

        task_->complete(*cmds, actions);
        for (auto& a : actions)
            do_action(state, std::move(a));
        
        return update_completion(state);
    }

    transition pause(engine::state& state)
    {
        LOG_D("Task ", ui_.id, " pause");

        if (ui_.state == states::paused ||
            ui_.state == states::error ||
            ui_.state == states::complete)
            return no_transition;

        for (auto& cmd : state.cmds)
        {
            if (cmd->task() != ui_.id)
                continue;
            cmd->cancel();
        }

        return goto_state(state, states::paused);
    }

    transition resume(engine::state& state)
    {
        LOG_D("Task ", ui_.id, " resume");

        if (ui_.state != states::paused)
            return no_transition;

        // it's possible that user paused the task but all 
        // command were already processed by the background
        // threads but are queued waiting for state transition.
        // then the task completes while it's in the paused state.
        // hence we expect the completion on resume and possibly
        // transfer to complete stated instead of waiting/queued.
        const auto transition = update_completion(state);
        if (transition)
            return transition;

        // if we were started before we go to waiting state.
        if (started_)
            return goto_state(state, states::waiting);

        // go back into queued state.
        return goto_state(state, states::queued);
    }

    void update(engine::state& state, ui::task& ui)
    {
        ui = ui_;                
        ui.etatime = 0;

        if (ui_.state == states::active || ui_.state == states::waiting)
        {
            if (ui_.runtime >= 10)
            {
                const auto complete  = ui.completion;
                const auto remaining = 100.0 - complete;
                const auto timespent = ui.runtime;
                const auto time_per_percent = timespent / complete;
                const auto timeremains = remaining * time_per_percent;
                ui_.etatime = timeremains;
            }
        }

    }

    transition on_action(engine::state& state, action* a)
    {
        std::unique_ptr<action> act(a);

        num_active_actions_--;

        if (ui_.state == states::error)
            return no_transition;

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

            return update_completion(state);
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

        LOG_E("Task ", ui_.id, " (", err.code.value(), ") ", err.code.message());
        LOG_E("Task ", ui_.id, " ", err.what);
        return goto_state(state, states::error);            
    }

    std::size_t account() const 
    { return ui_.account; }

    std::size_t tid() const
    { return ui_.id; }

    std::size_t bid() const 
    { return ui_.bid; }

    states state() const 
    { return ui_.state; }

    std::string desc() const 
    { return ui_.desc; }

    std::uint64_t size() const
    { return ui_.size; }

private:
    transition update_completion(engine::state& state)
    {
        // completion calculation needs to be done in the task because
        // only the task knows how many actions are to be performed per buffer.
        // i.e. we can't here reliably count 1 buffer as 1 step towards completion
        // since the completion updates only when the data is downloaded AND processed.
        ui_.completion = task_->completion();

        //num_complete_bytes_ = task_->bytes_complete();

        // if we have pending actions our state should not change
        // i.e. we're active (or possibly paused)
        if (num_active_actions_)
            return no_transition; 

        // if we have currently executing cmdlists our state should not change.
        // i.e. we're active (or possibly paused)
        if (num_active_cmdlists_)
            return no_transition;


        // if task list has more commands and there are no currently
        // executing cmdlists or actions and we're not paused
        // we'll transition into waiting state.
        if (task_->has_commands())
        {
            if (ui_.state == states::active)
                return goto_state(state, states::waiting);
        }
        else
        {
            if (ui_.state == states::finalize)
                return goto_state(state, states::complete);
            else if (ui_.state == states::active)
                return goto_state(state, states::finalize);
        }
        return no_transition;
    }

    void do_action(engine::state& state, std::unique_ptr<action> a) 
    {
        if (!a) return;

        a->set_id(ui_.id);

        state.submit(a.release());

        num_active_actions_++;
    }

    transition goto_state(engine::state& state, states new_state)
    {
        const auto old_state = ui_.state;

        switch (new_state)
        {
            case states::active:
                if (!started_)
                {
                    LOG_D("Task ", ui_.id,  " starting...");
                    do_action(state, task_->start());
                    started_ = true;
                }
                ui_.state = new_state;
                break;

            case states::error:
                task_.release();
                ui_.state = new_state;
                break;

            case states::finalize:
                {
                    auto act = task_->finalize();
                    if (!act)
                        return goto_state(state, states::complete);
                    do_action(state, std::move(act));
                    ui_.state = new_state;
                }
                break;

            case states::complete:
                ui_.completion = 100.0;
                ui_.etatime    = 0;
                ui_.state      = new_state;
                state.bytes_ready += ui_.size;
                if (auto* ptr = dynamic_cast<class download*>(task_.get()))
                {
                    const auto& files = ptr->files();
                    for (const auto& file : files)
                    {
                        ui::file ui;
                        ui.binary  = file->is_binary();
                        ui.name    = file->name();
                        ui.path    = file->path();                        
                        ui.size    = file->size();                        
                        ui.damaged = ui_.error.any_bit();
                        state.on_file_callback(ui);
                    }
                }
                break;

            default:
                ui_.state = new_state;
                break;
        }

        LOG_D("Task ", ui_.id, " => ", str(new_state));
        LOG_D("Task ", ui_.id, " has ", num_active_cmdlists_,  " active cmdlists");
        LOG_D("Task ", ui_.id, " has ", num_active_actions_, " active actions");        

        return { old_state, new_state };
    }

private:
    ui::task ui_;    
    std::unique_ptr<newsflash::task> task_;
    std::size_t num_active_cmdlists_;
    std::size_t num_active_actions_;
    std::size_t num_complete_bytes_;
private:
    bool started_;
};


class engine::batch
{
public:
    using states = ui::task::states;

    batch(std::size_t account, std::size_t id, std::uint64_t size, std::size_t num_tasks, 
        std::string path, std::string desc) : num_tasks_(num_tasks), path_(std::move(path))
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
        bytes_complete_ = 0;

        std::memset(&statesets_, 0, sizeof(statesets_));
        statesets_[(int)states::queued] = num_tasks;

        LOG_I("Batch ", ui_.id, " (", ui_.desc, ") created");
    }
   ~batch()
    {
        LOG_I("Batch ", ui_.id, " deleted");
    }

    void pause(engine::state& state)
    {
        LOG_D("Batch pause ", ui_.id);

        for (auto& task : state.tasks)
        {
            if (task->bid() != ui_.id)
                continue;

            const auto transition = task->pause(state);
            if (transition)
            {
                leave_state(*task, transition.previous);
                enter_state(*task, transition.current);
            }
        }
        update(state);
    }

    void resume(engine::state& state)
    {
        LOG_D("Batch resume ", ui_.id);

        for (auto& task : state.tasks)
        {
            if (task->bid() != ui_.id)
                continue;

            const auto transition = task->resume(state);
            if (transition)
            {
                leave_state(*task, transition.previous);
                enter_state(*task, transition.current);
            }
        }
        update(state);
    }

    void update(engine::state& state, ui::task& ui)
    {
        ui = ui_;
        ui.etatime = 0;
        if (ui_.size)
        {
            ui.completion = 100.0 * (double(bytes_complete_) / double(ui_.size));
        }
        else
        {
            const auto num_complete = 
                statesets_[(int)states::complete] +
                statesets_[(int)states::error];

            ui.completion = 100.0 * (double(num_complete) / double(num_tasks_));
        }

        if (ui_.state == states::waiting || ui_.state == states::active)
        {
            if (ui_.runtime >= 10)
            {
                const auto complete = ui.completion;
                const auto remaining = 100.0 - complete;
                const auto timespent = ui.runtime;
                const auto time_per_percent = timespent / complete;
                const auto time_remains = remaining * time_per_percent;
                ui_.etatime = time_remains;
            }
        }
    }

    void kill(engine::state& state)
    {
        LOG_D("Batch kill ", ui_.id);

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

    bool kill(engine::state& state, const task& t)
    {
        leave_state(t, t.state());
        num_tasks_--;
        update(state);

        return num_tasks_ == 0;
    }

    void update(engine::state& state, const task& t, const task::transition& s)
    {
        leave_state(t, s.previous);
        enter_state(t, s.current);
        update(state);

        if (s.current == states::complete || s.current == states::error)
        {
            bytes_complete_ += t.size();
            if (ui_.state == states::complete)
            {
                ui::batch batch;
                batch.path = path_;
                state.on_batch_callback(batch);
            }
        }
    }

    void tick(engine::state& state, const std::chrono::milliseconds& elapsed)
    {
        if (ui_.state == states::active)
            ui_.runtime++;
    }

    std::size_t id() const 
    { return ui_.id; }

private:
    void enter_state(const task& t, states s)
    {
        statesets_[(int)s]++;
    }
    void leave_state(const task& t, states s)
    {
        assert(statesets_[(int)s]);

        statesets_[(int)s]--;
    }

    bool is_empty_set(states s)
    {
        return statesets_[(int)s] == 0;
    }

    bool is_full_set(states s)
    {
        return statesets_[(int)s] == num_tasks_;
    }
    void update(engine::state& state)
    {
        const auto num_states = std::accumulate(std::begin(statesets_),
            std::end(statesets_), 0);
        assert(num_states == num_tasks_);

        if (!is_empty_set(states::active))
            goto_state(state, states::active);
        else if (!is_empty_set(states::waiting))
            goto_state(state, states::waiting);
        if (!is_empty_set(states::paused))
            goto_state(state, states::paused);
        else if (is_full_set(states::complete))
            goto_state(state, states::complete);
        else if (is_full_set(states::error))
            goto_state(state, states::error);
        else if (is_full_set(states::queued))
            goto_state(state, states::queued);
    }

    void goto_state(engine::state& state, states new_state)
    {
        ui_.state = new_state;
        switch (new_state)
        {
            case states::complete:
                ui_.completion = 100.0;
                ui_.etatime    = 0;
                break;

            default:
            break;
        }

        LOG_D("Batch ", ui_.id, " => ", str(new_state));
        LOG_D("Batch has ", num_tasks_, " tasks");
        LOG_D("Batch items Queued: ", statesets_[(int)states::queued], 
                   " Waiting: ", statesets_[(int)states::waiting],
                   " Active: ", statesets_[(int)states::active],
                   " Paused: ", statesets_[(int)states::paused],
                   " Complete: ", statesets_[(int)states::complete],
                   " Errored: ", statesets_[(int)states::error]);
    }
private:
    ui::task ui_;
private:
    std::size_t num_tasks_;
    std::size_t statesets_[7];
    std::uint64_t bytes_complete_;
    std::string path_;
};

engine::batch& engine::state::find_batch(std::size_t id)
{
    auto it = std::find_if(std::begin(batches), std::end(batches),
        [&](const std::unique_ptr<batch>& b) {
            return b->id() == id;
        });
    assert(it != std::end(batches));
    return *(*it);
}

void engine::state::execute()
{
    if (!started)
        return;

    for (auto it = std::begin(tasks); it != std::end(tasks); ++it)
    {
        auto& task  = *it;
        if (!task->eligible_for_run())
            continue;

        std::size_t num_conns = std::count_if(std::begin(conns), std::end(conns),
            [&](const std::unique_ptr<engine::conn>& c) {
                return c->account() == task->account();
            });

        const auto& acc = find_account(task->account());
        for (auto i=num_conns; i<acc.connections; ++i)
            conns.emplace_back(new engine::conn(acc.id, oid++, *this));

        while (task->eligible_for_run())
        {
            auto it = std::find_if(std::begin(conns), std::end(conns),
                [&](const std::unique_ptr<engine::conn>& c) {
                    return c->is_ready() && (c->account() == task->account());
                });
            if (it == std::end(conns))
                break;

            const auto transition = task->run(*this);
            if (transition)
            {
                auto& batch = find_batch(task->bid());
                batch.update(*this, *task, transition);
            }
        }
    }
}

void engine::state::enqueue(const task& t, std::shared_ptr<cmdlist> cmd)
{
    cmds.push_back(cmd);

    if (!started)
        return;

    std::size_t num_conns = 0;

    for (auto& conn : conns)
    {
        if (conn->account() != cmd->account())
            continue;

        ++num_conns;

        if (!conn->is_ready())
            continue;

        cmd->set_conn(conn->id());
        conn->execute(*this, cmd, t.tid(), t.desc());
        return;
    }

    const auto& acc = find_account(cmd->account());
    if (num_conns < acc.connections)
    {
        const auto id = oid++;
        conns.emplace_back(new engine::conn(acc.id, id, *this));
    }

}


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
    state_->repartition_task_list = false;
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

void engine::set_fill_account(std::size_t id)
{
    state_->fill_account = id;
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

    std::unique_ptr<batch> batch(new class batch(aid, bid, batch_size, num_tasks, spec.path, spec.desc));
    state_->batches.push_back(std::move(batch));
    state_->execute();
}

bool engine::pump()
{
    for (;;)
    {
        auto* action = state_->get_action();
        if (!action)
            break;

        if (auto* e = dynamic_cast<class connection::execute*>(action))
        {
            auto cmds  = e->get_cmdlist();
            auto tid   = e->get_tid();
            auto bytes = e->get_bytes_transferred();

            state_->cmds.erase(std::find(std::begin(state_->cmds), std::end(state_->cmds), cmds));

            auto tit  = std::find_if(std::begin(state_->tasks), std::end(state_->tasks),
                [&](const std::unique_ptr<engine::task>& t) {
                    return t->tid() == tid;
                });
            if (tit != std::end(state_->tasks))
            {
                auto& task = *tit;
                const auto transition = task->complete(*state_, cmds);
                if (transition)
                {
                    auto& batch = state_->find_batch(task->bid());
                    batch.update(*state_, *task, transition);
                }
            }

            state_->bytes_downloaded += bytes;
        }
        else if (auto* w = dynamic_cast<class datafile::write*>(action))
        {
            auto bytes = w->get_write_size();
            if (!w->has_exception())
                state_->bytes_written += bytes;
        }

        const auto id = action->get_id();
        auto it = std::find_if(std::begin(state_->conns), std::end(state_->conns),
            [&](const std::unique_ptr<engine::conn>& c) {
                return c->id() == id;
            });
        if (it != std::end(state_->conns))
        {
            auto& conn = *it;
            conn->on_action(*state_, action);
            if (conn->is_ready())
            {
                auto it = std::find_if(std::begin(state_->cmds), std::end(state_->cmds),
                    [&](const std::shared_ptr<cmdlist>& c) {
                        return c->conn() == 0 &&
                               c->account() == conn->account() &&
                               c->is_canceled() == false;
                           });
                if (it != std::end(state_->cmds))
                {
                    auto cmd = *it;
                    auto tit = std::find_if(std::begin(state_->tasks), std::end(state_->tasks),
                        [&](const std::unique_ptr<task>& t) {
                            return t->tid() == cmd->task();
                        });
                    ASSERT(tit != std::end(state_->tasks));
                    auto& task = *tit;

                    conn->execute(*state_, cmd, task->tid(), task->desc());
                    cmd->set_conn(conn->id());
                }                
                else
                {
                    auto it = std::find_if(std::begin(state_->tasks), std::end(state_->tasks),
                        [&](const std::unique_ptr<engine::task>& t) {
                            return t->account() == conn->account() &&
                                   t->eligible_for_run();
                               });
                    if (it != std::end(state_->tasks))
                    {
                        auto& task = *it;
                        const auto transition = task->run(*state_);
                        if (transition)
                        {
                            auto& batch = state_->find_batch(task->bid());
                            batch.update(*state_, *task, transition);
                        }
                    }
                }                
            }
        }
        else
        {
            for (auto& task : state_->tasks)
            {
                if (task->tid() != id)
                    continue;

                const auto transition = task->on_action(*state_, action);
                if (transition)
                {
                    auto& batch = state_->find_batch(task->bid());
                    batch.update(*state_, *task, transition);
                }
                break;
            }
        }
        state_->num_pending_actions--;
        state_->logger->flush();        
    }

    return state_->num_pending_actions != 0;
}

void engine::tick()
{
    if (state_->repartition_task_list)
    {
        auto tit = std::begin(state_->tasks);

        for (auto& batch : state_->batches)
        {
            tit = std::stable_partition(tit, std::end(state_->tasks),
                [&](const std::unique_ptr<task>& t) {
                    return t->bid() == batch->id();
                });
        }

        state_->repartition_task_list = false;
    }

    for (auto& conn : state_->conns)
    {
    // todo: measure time here instead of relying on the clientr        
        conn->tick(*state_, std::chrono::seconds(1));
    }

    for (auto& task : state_->tasks)
    {
        task->tick(*state_, std::chrono::seconds(1));
    }

    for (auto& batch : state_->batches)
    {
        batch->tick(*state_, std::chrono::seconds(1));
    }

    if (state_->logger)
        state_->logger->flush();
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

        state_->execute();
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

void engine::set_batch_callback(on_batch batch_callback)
{
    state_->on_batch_callback = std::move(batch_callback);
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

    if (state_->group_items)
    {
        auto tit = std::begin(state_->tasks);

        for (auto& batch : state_->batches)
        {
            tit = std::stable_partition(tit, std::end(state_->tasks),
                [&](const std::unique_ptr<task>& t) {
                    return t->bid() == batch->id();
                });
        }        

        state_->repartition_task_list = false;
    }
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
        const auto& batches = state_->batches;

        tasklist.resize(batches.size());

        for (std::size_t i=0; i<batches.size(); ++i)
            batches[i]->update(*state_, tasklist[i]);
    }
    else
    {
        const auto& tasks = state_->tasks;

        tasklist.resize(tasks.size());

        for (std::size_t i=0; i<tasks.size(); ++i)
            tasks[i]->update(*state_, tasklist[i]);
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
        assert(i < state_->batches.size());

        auto it = state_->batches.begin();
        it += i;
        (*it)->kill(*state_);
        state_->batches.erase(it);
    }
    else
    {
        assert(i < state_->tasks.size());

        auto tit = state_->tasks.begin();
        tit += i;

        auto bit = std::find_if(std::begin(state_->batches), std::end(state_->batches),
            [&](const std::unique_ptr<batch>& b) {
                return b->id() == (*tit)->bid();
            });
        ASSERT(bit != std::end(state_->batches));

        auto& batch = *bit;
        auto& task  = *tit;
        if (batch->kill(*state_, *task))
        {
            state_->batches.erase(bit);
        }

        task->kill(*state_);        
        state_->tasks.erase(tit);
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
        assert(index < state_->batches.size());

        state_->batches[index]->pause(*state_);
    }
    else
    {
        assert(index < state_->tasks.size());

        auto& task  = state_->tasks[index];

        const auto transition = task->pause(*state_);            
        if (transition)
        {
            auto& batch = state_->find_batch(task->bid());            
            batch.update(*state_, *task, transition);
        }
    }
}

void engine::resume_task(std::size_t index)
{
    if (state_->group_items)
    {
        assert(index < state_->batches.size());

        state_->batches[index]->resume(*state_);
    }
    else
    {

        assert(index < state_->tasks.size());

        auto& task  = state_->tasks[index];

        const auto transition = task->resume(*state_);            
        if (transition)
        {
            auto& batch = state_->find_batch(task->bid());            
            batch.update(*state_, *task, transition);
        }

    }
    state_->execute();                        
}

void engine::move_task_up(std::size_t index)
{
    if (state_->group_items)
    {
        assert(state_->batches.size() > 1);
        assert(index < state_->batches.size());
        assert(index > 0);
        std::swap(state_->batches[index], state_->batches[index-1]);

        state_->repartition_task_list = true;
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
        assert(state_->batches.size() > 1);
        assert(index < state_->batches.size()-1);
        std::swap(state_->batches[index], state_->batches[index + 1]);

        state_->repartition_task_list = true;
    }
    else
    {
        assert(state_->tasks.size() > 1);
        assert(index < state_->tasks.size()-1);
        std::swap(state_->tasks[index], state_->tasks[index + 1]);
    }
}

} // newsflash
