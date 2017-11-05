// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include "engine.pb.h"
#include "newsflash/warnpop.h"

#include <algorithm>
#include <numeric> // for accumulate
#include <deque>
#include <mutex>
#include <fstream>
#include <list>
#include <cassert>
#include <cstdlib> // for getenv

#include "engine.h"
#include "connection.h"
#include "download.h"
#include "decode.h"
#include "action.h"
#include "assert.h"
#include "filesys.h"
#include "logging.h"
#include "utf8.h"
#include "threadpool.h"
#include "cmdlist.h"
#include "settings.h"
#include "datafile.h"
#include "listing.h"
#include "update.h"
#include "throttle.h"
#include "sslcontext.h"
#include "encoding.h"
#include "nntp.h"

namespace newsflash
{

#define CASE(x) case x: return #x

const char* str(buffer::status s)
{
    using state = buffer::status;
    switch (s)
    {
        CASE(state::none);
        CASE(state::success);
        CASE(state::unavailable);
        CASE(state::dmca);
        CASE(state::error);
    }
    assert(0);
    return nullptr;
}

const char* str(ui::TaskDesc::States s)
{
    using State = ui::TaskDesc::States;
    switch (s)
    {
        CASE(State::Queued);
        CASE(State::Waiting);
        CASE(State::Active);
        CASE(State::Paused);
        CASE(State::Complete);
        CASE(State::Error);
        CASE(State::Crunching);
    }
    assert(0);
    return nullptr;
}

const char* str(ui::Connection::States s)
{
    using state = ui::Connection::States;
    switch (s)
    {
        CASE(state::Disconnected);
        CASE(state::Resolving);
        CASE(state::Connecting);
        CASE(state::Initializing);
        CASE(state::Connected);
        CASE(state::Active);
        CASE(state::Error);
    }
    assert(0);
    return nullptr;
}

#undef CASE


struct Engine::State {
    std::deque<std::unique_ptr<TaskState>> tasks;
    std::deque<std::unique_ptr<ConnState>> conns;
    std::deque<std::unique_ptr<BatchState>> batches;

    std::vector<ui::Account> accounts;
    std::list<std::shared_ptr<cmdlist>> cmds;
    std::uint64_t bytes_downloaded = 0;
    std::uint64_t bytes_queued = 0;
    std::uint64_t bytes_ready = 0;
    std::uint64_t bytes_written = 0;
    std::size_t oid = 1; // object id for tasks/connections
    std::size_t fill_account = 0;
    std::string logpath;

    std::unique_ptr<Engine::Factory> factory;

    std::unique_ptr<class logger> logger;
    std::unique_ptr<ConnTestState> current_connection_test;

    std::mutex mutex;
    std::queue<std::unique_ptr<action>> actions;
    std::unique_ptr<threadpool> threads;
    std::size_t num_pending_actions = 0;
    std::size_t num_pending_tasks = 0;

    bool prefer_secure = true;
    bool overwrite_existing = false;
    bool discard_text = false;
    bool started = false;
    bool group_items = false;
    bool repartition_task_list = false;
    bool quit_pump_loop = false;

    Engine::on_error on_error_callback;
    Engine::on_file  on_file_callback;
    Engine::on_batch on_batch_callback;
    Engine::on_list  on_list_callback;
    Engine::on_task  on_task_callback;
    Engine::on_update on_update_callback;
    Engine::on_header_data on_header_data_callback;
    Engine::on_header_info on_header_info_callback;
    Engine::on_async_notify on_notify_callback;
    Engine::on_finish on_finish_callback;
    Engine::on_quota on_quota_callback;
    Engine::on_conn_test on_test_callback;
    Engine::on_conn_test_log on_test_log_callback;

    throttle ratecontrol;

    State()
    {
        ratecontrol.set_quota(std::numeric_limits<std::size_t>::max());
        threads.reset(new threadpool(4));
        threads->on_complete = [&](action* a)
        {
            std::lock_guard<std::mutex> lock(mutex);
            actions.emplace(a);
            on_notify_callback();
        };
    }

   ~State()
    {
        tasks.clear();
        conns.clear();
        batches.clear();
        set_thread_log(nullptr);
    }

    const ui::Account& find_account(std::size_t id) const
    {
        auto it = std::find_if(accounts.begin(), accounts.end(),
            [=](const ui::Account& a) {
                return a.id == id;
            });
        ASSERT(it != std::end(accounts));

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
        if (a->get_affinity() == action::affinity::gui_thread)
        {
            LOG_D("Action ", a->get_id(), " (", a->describe(),  ") perform by current thread.");

            a->perform();
            std::lock_guard<std::mutex> lock(mutex);
            actions.emplace(a);
            on_notify_callback();
            quit_pump_loop = true;
        }
        else
        {
            LOG_D("Action ", a->get_id(), " (", a->describe(), ") submitted to the threadpool.");

            threads->submit(a);
        }
        num_pending_actions++;
    }

    void submit(action* a, threadpool::worker* thread)
    {
        threads->submit(a, thread);
        num_pending_actions++;
    }

    std::unique_ptr<action> get_action()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (actions.empty())
            return nullptr;
        std::unique_ptr<action> act = std::move(actions.front());
        actions.pop();

        LOG_D("Action ", act->get_id(), " (", act->describe(), ") is complete");
        LOG_D("Action ", act->get_id(), " has exception: ", act->has_exception());
        return act;
    }

    Engine::BatchState* find_batch(std::size_t id);

    void execute();
    void enqueue(const TaskState& t, std::shared_ptr<cmdlist> cmd);
};


class Engine::ConnState
{
private:
    ConnState(Engine::State& state, std::size_t cid)
    {
        std::string file;
        std::string name;
        for (int i=0; i<1000; ++i)
        {
            name = str("connection", i, ".log");
            file = fs::joinpath(state.logpath, name);
            auto it = std::find_if(std::begin(state.conns), std::end(state.conns),
                [&](const std::unique_ptr<ConnState>& c) {
                    return c->ui_.logfile == file;
                });
            if (it == std::end(state.conns))
                break;
        }

        ui_.error      = errors::None;
        ui_.state      = states::Disconnected;
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

    using states = ui::Connection::States;
    using errors = ui::Connection::Errors;

public:
    ConnState(Engine::State& state, std::size_t aid, std::size_t cid) : ConnState(state, cid)
    {
        const auto& acc = state.find_account(aid);

        connection::spec spec;
        spec.pthrottle = &state.ratecontrol;
        spec.password  = acc.password;
        spec.username  = acc.username;
        spec.enable_compression = acc.enable_compression;
        spec.enable_pipelining = acc.enable_pipelining;
        if ((state.prefer_secure && acc.enable_secure_server) ||
            (acc.enable_general_server == false))
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

        conn_ = state.factory->AllocateConnection();

        do_action(state, conn_->connect(spec));

        LOG_I("Connection ", ui_.id, " ", ui_.host, ":", ui_.port);
    }

    ConnState(Engine::State& state, std::size_t cid, const ConnState& dna) : ConnState(state, cid)
    {
        ui_.account = dna.ui_.account;
        ui_.host    = dna.ui_.host;
        ui_.port    = dna.ui_.port;
        ui_.secure  = dna.ui_.secure;
        ui_.account = dna.ui_.account;

        const auto& acc = state.find_account(dna.ui_.account);

        connection::spec spec;
        spec.pthrottle = &state.ratecontrol;
        spec.password  = acc.password;
        spec.username  = acc.username;
        spec.enable_compression = acc.enable_compression;
        spec.enable_pipelining  = acc.enable_pipelining;
        spec.hostname = dna.ui_.host;
        spec.hostport = dna.ui_.port;
        spec.use_ssl  = dna.ui_.secure;

        conn_ = state.factory->AllocateConnection();
        do_action(state, conn_->connect(spec));

        LOG_I("Connection ", ui_.id, " ", ui_.host, ":", ui_.port);
    }

   ~ConnState()
    {
        LOG_I("Connection ", ui_.id, " deleted");
    }

    void tick(Engine::State& state, const std::chrono::milliseconds& elapsed)
    {
        if (ui_.state == states::Connected)
        {
            if (--ticks_to_ping_ == 0)
                do_action(state, conn_->ping());

        }
        else if (ui_.state == states::Error)
        {
            if (ui_.error == errors::AuthenticationRejected ||
                ui_.error == errors::NoPermission)
                return;

            if (--ticks_to_conn_)
                return;

            LOG_D("Connection ", ui_.id, " reconnecting...");
            ui_.error = errors::None;
            ui_.state = states::Disconnected;

            const auto& acc = state.find_account(ui_.account);
            connection::spec s;
            s.pthrottle = &state.ratecontrol;
            s.password = acc.password;
            s.username = acc.username;
            s.use_ssl  = ui_.secure;
            s.hostname = ui_.host;
            s.hostport = ui_.port;
            s.enable_compression = acc.enable_compression;
            s.enable_pipelining  = acc.enable_pipelining;
            do_action(state, conn_->connect(s));
        }
    }

    void stop(Engine::State& state)
    {
        if (ui_.state == states::Disconnected)
            return;

        conn_->cancel();
        if (ui_.state == states::Connected ||
            ui_.state == states::Active)
        {
            do_action(state, conn_->disconnect());
        }

        state.threads->detach(thread_);
    }

    void execute(Engine::State& state, std::shared_ptr<cmdlist> cmds, std::size_t tid, std::string desc)
    {
        ui_.task  = tid;
        ui_.desc  = std::move(desc);
        ui_.state = states::Active;
        LOG_I("Connection ", ui_.id, " executing cmdlist ", cmds->id());

        do_action(state, conn_->execute(cmds, tid));
    }

    void update(ui::Connection& ui)
    {
        ui = ui_;
        ui.bps = 0;
        if (ui_.state == states::Active)
        {
            // if the bytes value hasn't increased since the last
            // read then the bps value cannot be valid but has become
            // stale since the connection has not ready any data. (i.e. it's stalled)
            const auto bytes_before  = ui_.down;
            const auto bytes_current = conn_->num_bytes_transferred();
            if (bytes_current != bytes_before)
            {
                ui_.bps  = ui.bps  = conn_->current_speed_bps();
                ui_.down = ui.down = bytes_current;
            }
        }
    }

    void on_action(Engine::State& state, std::unique_ptr<action> act)
    {
        LOG_D("Connection ", ui_.id, " action ", act->get_id(), "(", act->describe(), ") complete");

        assert(act->get_owner() == ui_.id);

        ticks_to_ping_ = 30;
        ticks_to_conn_ = 5;

        if (act->has_exception())
        {
            ui_.state = states::Error;
            ui_.bps   = 0;
            ui_.task  = 0;
            ui_.desc  = "";
            ui::SystemError error;
            error.resource = ui_.host;

            // todo: refactor the connection errors to non-exceptions.
            try
            {
                act->rethrow();
            }
            catch (const connection::exception& e)
            {
                switch (e.error())
                {
                    case connection::error::resolve:
                        ui_.error = ui::Connection::Errors::Resolve;
                        break;
                    case connection::error::authentication_rejected:
                        ui_.error = ui::Connection::Errors::AuthenticationRejected;
                        break;
                    case connection::error::no_permission:
                        ui_.error = ui::Connection::Errors::NoPermission;
                        break;
                    case connection::error::network:
                        ui_.error = ui::Connection::Errors::Network;
                        break;
                    case connection::error::timeout:
                        ui_.error = ui::Connection::Errors::Timeout;
                        break;
                    case connection::error::pipeline_reset:
                        ui_.error = ui::Connection::Errors::Other;
                        break;
                }
                error.what = e.what();
            }
            catch (const std::system_error& e)
            {
                const auto code = e.code();
                if (code == std::errc::connection_refused)
                    ui_.error = ui::Connection::Errors::Refused;
                else ui_.error = ui::Connection::Errors::Other;

                error.code = code;
                error.what = e.what();
            }
            catch (const std::exception &e)
            {
                ui_.error = errors::Other;
                error.what  = e.what();
            }

            LOG_E("Connection ", ui_.id, " (", error.code.value(), ") ", error.code.message());
            LOG_E("Connection ", ui_.id, " ", error.what);

            state.on_error_callback(error);
            state.logger->flush();
            logger_->flush();
            return;
        }

        auto next = conn_->complete(std::move(act));
        if (next)
            do_action(state, std::move(next));
        else if (ui_.state == states::Initializing)
            ui_.state = states::Connected;
        else if (ui_.state == states::Active)
        {
           ui_.state = states::Connected;
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
        return ui_.state == states::Connected;
    }

    double bps() const
    {
        if (ui_.state == states::Active)
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
    { return conn_->username(); }

    const std::string& password() const
    { return conn_->password(); }

private:
    void do_action(Engine::State& state, std::unique_ptr<action> a)
    {
        auto* ptr = a.get();

        if (dynamic_cast<class connection::resolve*>(ptr))
            ui_.state = states::Resolving;
        else if (dynamic_cast<class connection::connect*>(ptr))
            ui_.state = states::Connecting;
        else if (dynamic_cast<class connection::initialize*>(ptr))
            ui_.state = states::Initializing;
        else if (dynamic_cast<class connection::execute*>(ptr))
            ui_.state = states::Active;
        else if (dynamic_cast<class connection::disconnect*>(ptr))
            ui_.state = states::Disconnected;

        LOG_D("Connection ", ui_.id,  " => ", str(ui_.state));
        LOG_D("Connection ", ui_.id, " current task ", ui_.task, " (", ui_.desc, ")");
        LOG_D("Connection ", ui_.id, " new action ", a->get_id(), "(", a->describe(), ")");

        a->set_owner(ui_.id);
        a->set_log(logger_);
        state.submit(a.release(), thread_);
    }

private:
    ui::Connection ui_;
    std::unique_ptr<connection> conn_;
    std::shared_ptr<logger> logger_;
    threadpool::worker* thread_;
    unsigned ticks_to_ping_ = 0;
    unsigned ticks_to_conn_ = 0;
};


// similar to engine::conn but one shot only.
// used for connection tests only.
class Engine::ConnTestState
{
public:
    ConnTestState(const ui::Account& account, std::size_t id, Engine::State& state)
    {
        connection::spec spec;
        spec.pthrottle = nullptr;
        spec.password = account.password;
        spec.username = account.username;
        spec.enable_compression = account.enable_compression;
        spec.enable_pipelining = account.enable_pipelining;
        spec.authenticate_immediately = true;
        if (account.enable_secure_server)
        {
            spec.hostname = account.secure_host;
            spec.hostport = account.secure_port;
            spec.use_ssl  = true;
        }
        else
        {
            spec.hostname = account.general_host;
            spec.hostport = account.general_port;
            spec.use_ssl  = false;
        }
        logger_ = std::make_shared<buffer_logger>();
        id_     = id;
        success_ = false;
        finished_ = false;

        do_action(state, conn_.connect(spec));
    }
    void on_action(Engine::State& state, std::unique_ptr<action> act)
    {
        assert(act->get_owner() == id_);

        if (state.on_test_log_callback)
        {
            const auto& msgs = logger_->lines();
            for (const auto& msg : msgs)
                state.on_test_log_callback(msg);
        }
        logger_->clear();

        if (act->has_exception())
        {
            success_  = false;
            finished_ = true;
            try
            {
                act->rethrow();
            }
            catch (const std::exception& e)
            {}
            if (state.on_test_callback)
                state.on_test_callback(false);
            return;
        }

        auto next = conn_.complete(std::move(act));
        if (next)
        {
            do_action(state, std::move(next));
        }
        else
        {
            success_  = true;
            finished_ = true;
            if (state.on_test_callback)
                state.on_test_callback(success_);
        }
    }

    bool is_finished() const
    { return finished_; }

    bool is_successful() const
    { return success_; }

    std::size_t id() const
    { return id_; }

private:
    void do_action(Engine::State& state, std::unique_ptr<action> act)
    {
        act->set_owner(id_);
        act->set_log(logger_);
        state.submit(act.release());
    }

private:
    connection conn_;
    std::shared_ptr<buffer_logger> logger_;
    std::size_t id_;
private:
    bool success_;
    bool finished_;
};

class Engine::TaskState
{
    TaskState(std::size_t id) : num_active_cmdlists_(0), num_active_actions_(0), num_actions_ready_(0), num_actions_total_(0)
    {
        ui_.task_id  = id;
        ui_.size     = 0;
        is_fillable_ = false;
        num_bytes_queued_ = 0;
    }

public:
    using states = ui::TaskDesc::States;
    using clock  = std::chrono::steady_clock;

    struct transition {
        states previous;
        states current;

        operator bool() const
        {
            return previous != current;
        }
    };

    //constexpr static auto no_transition = transition{states::queued, states::queued};
    const static transition no_transition;

    TaskState(Engine::State& s, std::size_t id, const ui::FileDownload& file) : TaskState(id)
    {
        ui_.state      = states::Queued;
        ui_.desc       = file.name;
        ui_.size       = file.size;
        ui_.path       = file.path;
        is_fillable_   = ui::is_fillable(file);

        LOG_D("Task: download has ", file.articles.size(), " articles");
        LOG_D("Task: download path: '", file.path, "'");

        task_ = s.factory->AllocateTask(file);

        num_actions_total_ = task_->max_num_actions();

        LOG_D("Task: download");
        LOG_D("Task: is_fillable: ", is_fillable_);
        LOG_I("Task ", ui_.task_id, " (", ui_.desc, ") created");
    }

    TaskState(Engine::State& s, std::size_t id, const ui::GroupListDownload& list) : TaskState(id)
    {
        ui_.desc = list.desc;

        task_ = s.factory->AllocateTask(list);

        num_actions_total_ = task_->max_num_actions();

        LOG_D("Task: listing");
        LOG_I("Task ", ui_.task_id, " (", ui_.desc, ") created");
    }

    TaskState(Engine::State& s, std::size_t id, const ui::HeaderDownload& download) : TaskState(id)
    {
        ui_.desc = download.desc;

        auto task = s.factory->AllocateTask(download);
        if (auto* ptr = dynamic_cast<class update*>(task.get()))
        {
            // todo: refactor this.
            ptr->on_write = s.on_header_data_callback;
            ptr->on_info  = s.on_header_info_callback;
        }
        task_ = std::move(task);

        num_actions_total_ = task_->max_num_actions();
        LOG_D("Task: update");
        LOG_I("Task ", ui_.task_id, "( ", ui_.desc, ") created");
    }

    TaskState(Engine::State& s, const data::Download& spec) : TaskState(spec.task_id())
    {
        ui_.batch_id   = spec.batch_id();
        ui_.account    = spec.account_id();
        ui_.desc       = spec.desc();
        ui_.size       = spec.size();
        ui_.path       = spec.path();
        is_fillable_   = spec.enable_fill();

        ui::FileDownload file;
        file.path = spec.path();
        file.name = spec.desc();
        for (int i=0; i<spec.group_size(); ++i)
            file.groups.push_back(spec.group(i));
        for (int i=0; i<spec.article_size(); ++i)
            file.articles.push_back(spec.article(i));

        task_ = s.factory->AllocateTask(file);

        auto* ptr = dynamic_cast<download*>(task_.get());
        for (int i=0; i<spec.file_size(); ++i)
        {
            const auto& data = spec.file(i);
            const auto& path = data.filepath();
            const auto& name = data.filename();
            auto file = std::make_shared<datafile>(path, name,
                data.dataname(), data.is_binary());
            ptr->add_file(file);
            LOG_D("Task: Continue downloading file: '", fs::joinpath(path, name), "'");
        }

        num_actions_total_ = spec.num_actions_total();
        num_actions_ready_ = spec.num_actions_ready();
        ui_.completion = (double)num_actions_ready_ / (double)num_actions_total_ * 100.0;
        LOG_I("Task ", ui_.task_id, "( ", ui_.desc, ") restored");
    }

   ~TaskState()
    {
        ASSERT(locking_ == LockState::Unlocked);
        LOG_I("Task ", ui_.task_id, " deleted");
    }

    void lock(Engine::State& state)
    {
        ASSERT(locking_ == LockState::Unlocked);
        task_->lock();
        locking_ = LockState::Locked;
    }

    void unlock(Engine::State& state)
    {
        ASSERT(locking_ == LockState::Locked);
        task_->unlock();
        locking_ = LockState::Unlocked;
    }

    void serialize(data::TaskList& list)
    {
        if (ui_.state == states::Error || ui_.state == states::Complete)
            return;

        // this I guess shouldn't really matter.
        //assert(num_active_cmdlists_ == 0);
        assert(num_active_actions_  == 0);

        if (auto* ptr = dynamic_cast<download*>(task_.get()))
        {
            auto* spec = list.add_download();
            spec->set_account_id(ui_.account);
            spec->set_batch_id(ui_.batch_id);
            spec->set_task_id(ui_.task_id);
            spec->set_desc(ui_.desc);
            spec->set_size(ui_.size);
            spec->set_path(ptr->path());
            spec->set_num_actions_total(num_actions_total_);
            spec->set_num_actions_ready(num_actions_ready_);
            spec->set_enable_fill(is_fillable_);

            const auto& grouplist = ptr->groups();
            const auto& articles  = ptr->articles();
            for (const auto& g : grouplist)
                spec->add_group(g);

            for (const auto& a : articles)
                spec->add_article(a);

            const auto& files = ptr->files();
            for (const auto& f : files)
            {
                auto* file_data = spec->add_file();
                file_data->set_filename(f->filename());
                file_data->set_filepath(f->filepath());
                file_data->set_dataname(f->binary_name());
                file_data->set_is_binary(f->is_binary());
            }
        }
    }

    void kill(Engine::State& state)
    {
        LOG_D("Task ", ui_.task_id, " kill");

        if (ui_.state != states::Complete)
        {
            for (auto i=std::begin(state.cmds); i != std::end(state.cmds);)
            {
                auto cmd = *i;
                if (cmd->task() == ui_.task_id)
                {
                    cmd->cancel();
                    i = state.cmds.erase(i);
                }
                else
                {
                    ++i;
                }
            }

            assert(task_);
            task_->cancel();

            state.bytes_queued -= ui_.size;
        }
        if (!(ui_.state == states::Complete || ui_.state == states::Error))
        {
            state.num_pending_tasks--;
            if (state.num_pending_tasks == 0)
            {
                LOG_D("All tasks are complete");
                if (state.on_finish_callback)
                    state.on_finish_callback();
            }
        }
    }

    void configure(const settings& s)
    {
        if (task_)
            task_->configure(s);
    }

    void tick(Engine::State& state, const std::chrono::milliseconds& elapsed)
    {
        // todo: actually measure the time
        if (ui_.state == states::Active || ui_.state == states::Crunching)
            ui_.runtime++;
    }

    bool eligible_for_run() const
    {
        if (ui_.state == states::Waiting ||
            ui_.state == states::Queued ||
            ui_.state == states::Active)
        {
            return task_->has_commands();
        }
        return false;
    }

    transition run(Engine::State& state)
    {
        if (ui_.state == states::Complete ||
            ui_.state == states::Error ||
            ui_.state == states::Paused)
            return no_transition;

        auto cmds = task_->create_commands();

        LOG_I("Task ", ui_.task_id, " new cmdlist ", cmds->id());

        cmds->set_account(ui_.account);
        cmds->set_task(ui_.task_id);
        state.enqueue(*this, cmds);

        num_active_cmdlists_++;

        return goto_state(state, states::Active);
    }

    transition complete(Engine::State& state, std::shared_ptr<cmdlist> cmds)
    {
        assert(cmds->task() == ui_.task_id);

        if (!cmds->is_canceled())
        {
            assert(num_active_cmdlists_ > 0);
            num_active_cmdlists_--;
        }

        LOG_D("Task ", ui_.task_id, " receiving cmdlist ", cmds->id());

        if (ui_.state == states::Error)
            return no_transition;

        if (!cmds->is_good())
        {
            LOG_W("Task ", ui_.task_id, " cmdlist ", cmds->id(),  " failbit is on.");

            if (is_fillable_ && state.fill_account && (state.fill_account != cmds->account()))
            {
                LOG_I("Task ", ui_.task_id, " cmdlist ", cmds->id(), " set for refill");
                cmds->set_account(state.fill_account);
                state.enqueue(*this, cmds);
                ++num_active_cmdlists_;
                return no_transition;
            }
            else
            {
                ui_.error.set(ui::TaskDesc::Errors::Incomplete);
                return no_transition;
            }
        }
        else if (cmds->cmdtype() != cmdlist::type::groupinfo)
        {
            const auto& buffers  = cmds->get_buffers();
            const auto& commands = cmds->get_commands();
            //assert(commands.size() >= buffers.size());

            for (std::size_t i=0; i<buffers.size(); ++i)
            {
                const auto& buff = buffers[i];
                const auto status = buff.content_status();
                const auto& cmd   = (i < commands.size()) ? commands[i] : str("cmd", i);
                LOG_D("Task ", ui_.task_id, " command ", cmd, " status ", str(status));

                if (status == buffer::status::success)
                    continue;
                else if (status == buffer::status::error)
                {
                    if (state.on_error_callback)
                    {
                        ui::SystemError error;
                        error.resource = ui_.desc;
                        error.what     = "Data buffer error";
                        state.on_error_callback(error);
                    }
                    LOG_E("Task ", ui_.task_id, " Error: data buffer error");
                    return goto_state(state, states::Error);
                }

                if (is_fillable_ && state.fill_account && (state.fill_account != cmds->account()))
                {
                    LOG_I("Task ", ui_.task_id, " cmdlist ", cmds->id(), " set for refill.");
                    cmds->set_account(state.fill_account);
                    state.enqueue(*this, cmds);
                    ++num_active_cmdlists_;
                    return no_transition;
                }

                if (status == buffer::status::unavailable)
                    ui_.error.set(ui::TaskDesc::Errors::Incomplete);
                else if (status == buffer::status::dmca)
                    ui_.error.set(ui::TaskDesc::Errors::Dmca);
            }
        }

        std::vector<std::unique_ptr<action>> actions;

        ui::SystemError error;
        error.resource = ui_.desc;
        try
        {
            task_->complete(*cmds, actions);

            for (auto& a : actions)
                do_action(state, std::move(a));

            return update_completion(state);
        }
        catch (const std::system_error& e)
        {
            error.code = e.code();
            error.what = e.what();
        }
        catch (const std::exception& e)
        {
            error.what = e.what();
        }

        if (state.on_error_callback)
        {
            state.on_error_callback(error);
        }
        LOG_E("Task ", ui_.task_id, " Error: ", error.what);
        return goto_state(state, states::Error);
    }

    transition pause(Engine::State& state)
    {
        LOG_D("Task ", ui_.task_id, " pause");

        if (ui_.state == states::Paused ||
            ui_.state == states::Error ||
            ui_.state == states::Complete)
            return no_transition;

        for (auto it = std::begin(state.cmds); it != std::end(state.cmds); )
        {
            auto cmd = *it;
            if (cmd->task() == ui_.task_id)
            {
                cmd->cancel();
                it = state.cmds.erase(it);
                assert(num_active_cmdlists_ > 0);
                --num_active_cmdlists_;
            }
            else
            {
                ++it;
            }
        }

        return goto_state(state, states::Paused);
    }

    transition resume(Engine::State& state)
    {
        LOG_D("Task ", ui_.task_id, " resume");

        if (ui_.state != states::Paused)
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
        if (ui_.runtime)
            return goto_state(state, states::Waiting);

        // go back into queued state.
        return goto_state(state, states::Queued);
    }

    void update(Engine::State& state, ui::TaskDesc& ui)
    {
        ui = ui_;
        ui.etatime = 0;

        if (ui_.state == states::Active || ui_.state == states::Waiting || ui_.state == states::Crunching)
        {
            if (ui_.runtime >= 10 && ui_.completion > 0.0)
            {
                const auto complete  = ui.completion;
                const auto remaining = 100.0 - complete;
                const auto timespent = ui.runtime;
                const auto time_per_percent = timespent / complete;
                const auto timeremains = remaining * time_per_percent;
                ui.etatime = (std::uint32_t)timeremains;
            }
        }
    }

    transition on_action(Engine::State& state, std::unique_ptr<action> act)
    {
        const auto size = act->size();

        assert(act->get_owner() == ui_.task_id);
        assert(num_active_actions_ > 0);
        assert(size <= num_bytes_queued_);

        num_active_actions_--;
        num_actions_ready_++;
        num_bytes_queued_ -= size;

        LOG_D("Task ", ui_.task_id, " receiving action ", act->get_id(), " (", act->describe(), ")");
        LOG_D("Task ", ui_.task_id, " num_active_actions ", num_active_actions_);
        LOG_D("Task ", ui_.task_id, " num_actions_ready ", num_actions_ready_);
        LOG_D("Task ", ui_.task_id, " num_active_cmdlists ", num_active_cmdlists_);
        LOG_D("Task ", ui_.task_id, " has ", num_bytes_queued_ / (1024.0*1024.0), " Mb queued");

        if (ui_.state == states::Error)
            return no_transition;

        ui::SystemError error;
        error.resource = ui_.desc;
        try
        {
            if (act->has_exception())
                act->rethrow();

            if (auto* dec = dynamic_cast<decode*>(act.get()))
            {
                const auto err = dec->get_errors();
                if (err.test(decode::error::crc_mismatch))
                    ui_.error.set(ui::TaskDesc::Errors::Damaged);
                if (err.test(decode::error::size_mismatch))
                    ui_.error.set(ui::TaskDesc::Errors::Damaged);
            }

            std::vector<std::unique_ptr<action>> actions;

            task_->complete(*act, actions);
            act.reset();

            for (auto& a : actions)
                do_action(state, std::move(a));

            return update_completion(state);
        }
        catch (const std::system_error& e)
        {
            error.code = e.code();
            error.what = e.what();
        }
        catch (const std::exception& e)
        {
            error.what = e.what();
        }

        if (state.on_error_callback)
        {
            state.on_error_callback(error);
        }

        LOG_E("Task ", ui_.task_id, " Error: ", error.what);
        return goto_state(state, states::Error);
    }

    void set_account(std::size_t acc)
    { ui_.account = acc; }

    void set_batch(std::size_t batch)
    { ui_.batch_id = batch; }

    std::size_t account() const
    { return ui_.account; }

    std::size_t tid() const
    { return ui_.task_id; }

    std::size_t bid() const
    { return ui_.batch_id; }

    states state() const
    { return ui_.state; }

    std::string desc() const
    { return ui_.desc; }

    std::uint64_t size() const
    { return ui_.size; }

    double done() const
    { return ui_.completion; }

    bitflag<ui::TaskDesc::Errors> errors() const
    { return ui_.error; }

    bool is_valid() const
    {
        return (ui_.account != 0) &&
               (ui_.task_id != 0) &&
               (ui_.batch_id != 0);
    }



private:
    transition update_completion(Engine::State& state)
    {
        // completion calculation needs to be done in the task because
        // only the task knows how many actions are to be performed per buffer.
        // i.e. we can't here reliably count 1 buffer as 1 step towards completion
        // since the completion updates only when the data is downloaded AND processed.
        //assert(num_actions_total_);
        //assert(num_actions_ready_ <= num_actions_total_);
        if (num_actions_total_ == 0)
            num_actions_total_ = task_->max_num_actions();

        if (num_actions_total_)
        {
            ui_.completion = (double)num_actions_ready_ / (double)num_actions_total_ * 100.0;
            if (ui_.completion > 100.0)
                ui_.completion = 100.0;
        }

        // if we have pending actions our state should not change
        // i.e. we're active (or possibly paused)
        if (num_active_actions_)
        {
            if (num_bytes_queued_ >= MB(8))
            {
                if (ui_.state == states::Active)
                    return goto_state(state, states::Crunching);
            }
            return no_transition;
        }

        // if we have currently executing cmdlists our state should not change.
        // i.e. we're active (or possibly paused)
        if (num_active_cmdlists_)
            return no_transition;


        // if task list has more commands and there are no currently
        // executing cmdlists or actions and we're not paused
        // we'll transition into waiting state.
        if (task_->has_commands())
        {
            if (ui_.state == states::Active ||
                ui_.state == states::Crunching)
                return goto_state(state, states::Waiting);
        }
        else
        {
            return goto_state(state, states::Complete);
        }
        return no_transition;
    }

    void do_action(Engine::State& state, std::unique_ptr<action> a)
    {
        if (!a) return;

        a->set_owner(ui_.task_id);

        num_active_actions_++;
        num_bytes_queued_ += a->size();

        //LOG_D("Task ", ui_.task_id, " has a new active action ", typeid(*a).name());
        //LOG_D("Task ", ui_.task_id, " has ", num_active_actions_, " active actions");
        //state.logger->flush();

        state.submit(a.release());
    }

    transition goto_state(Engine::State& state, states new_state)
    {
        const auto old_state = ui_.state;

        switch (new_state)
        {
            case states::Queued:
                ui_.state   = new_state;
                ui_.etatime = 0;
                break;

            case states::Waiting:
                ui_.state = new_state;
                break;

            case states::Active:
                ui_.state = new_state;
                break;

            case states::Paused:
                ui_.state   = new_state;
                ui_.etatime = 0;
                break;

            case states::Crunching:
                ui_.state   = new_state;
                for (auto it = std::begin(state.cmds); it != std::end(state.cmds);)
                {
                    auto cmd = *it;
                    if (cmd->task() == ui_.task_id)
                    {
                        cmd->cancel();
                        it = state.cmds.erase(it);
                        assert(num_active_cmdlists_ > 0);
                        --num_active_cmdlists_;
                    }
                    else
                    {
                        ++it;
                    }

                }
                break;

            case states::Complete:
            {
                ASSERT(task_);
                ASSERT(task_->has_commands() == false);
                ASSERT(num_active_cmdlists_ == 0);
                ASSERT(num_active_actions_ == 0);

                task_->commit();

                ui_.state   = new_state;
                ui_.etatime = 0;
                if (state.on_task_callback)
                    state.on_task_callback(ui_);

                state.bytes_ready += ui_.size;

                auto results = state.factory->MakeResult(*task_, ui_);
                for (auto& result : results)
                {
                    ui::Result* result_ptr = result.get();

                    if (auto* ptr = dynamic_cast<ui::FileResult*>(result_ptr))
                    {
                        state.on_file_callback(*ptr);
                    }
                    else if (auto* ptr = dynamic_cast<ui::GroupListResult*>(result_ptr))
                    {
                        state.on_list_callback(*ptr);
                    }
                    else if (auto* ptr = dynamic_cast<ui::HeaderResult*>(result_ptr))
                    {
                        state.on_update_callback(*ptr);
                    }
                }
            }
            break;

            case states::Error:
                ui_.state   = new_state;
                ui_.etatime = 0;
                if (state.on_task_callback)
                    state.on_task_callback(ui_);

                for (auto i=std::begin(state.cmds); i != std::end(state.cmds); ++i)
                {
                    auto cmd = *i;
                    if (cmd->task() != ui_.task_id)
                        continue;
                    cmd->cancel();
                }
                break;
        }

        LOG_D("Task ", ui_.task_id, " => ", str(new_state));
        LOG_D("Task ", ui_.task_id, " has ", num_active_cmdlists_,  " active cmdlists");
        LOG_D("Task ", ui_.task_id, " has ", num_active_actions_, " active actions");
        LOG_FLUSH();

        if (new_state == states::Error || new_state == states::Complete)
        {
            state.num_pending_tasks--;
            if (state.num_pending_tasks == 0)
            {
                LOG_D("All tasks are complete");
                if (state.on_finish_callback)
                    state.on_finish_callback();
            }
        }

        return { old_state, new_state };
    }

private:
    ui::TaskDesc ui_;
private:
    std::unique_ptr<newsflash::task> task_;
    std::size_t num_active_cmdlists_ = 0;
    std::size_t num_active_actions_  = 0;
    std::size_t num_actions_ready_   = 0;
    std::size_t num_actions_total_   = 0;
    std::size_t num_bytes_queued_    = 0;
private:
    bool is_fillable_ = false;
private:
    enum class LockState {
        Locked, Unlocked
    };
    LockState locking_ = LockState::Unlocked;
};

// for msvc...
const Engine::TaskState::transition Engine::TaskState::no_transition {Engine::TaskState::states::Queued, Engine::TaskState::states::Queued};

class Engine::BatchState
{
    BatchState(std::size_t id)
    {
        ui_.batch_id = id;
        ui_.task_id  = id;
        std::fill(std::begin(statesets_), std::end(statesets_), 0);
        filebatch_  = false;
        num_slices_ = 0;
        num_tasks_  = 0;
    }

public:
    using states = ui::TaskDesc::States;

    BatchState(std::size_t id, const ui::FileBatchDownload& files) : BatchState(id)
    {
        ui_.account    = files.account;
        ui_.desc       = files.desc;
        ui_.path       = files.path;
        ui_.size       = std::accumulate(std::begin(files.files), std::end(files.files), std::uint64_t(0),
            [](std::uint64_t val, const ui::FileDownload& next) {
                return val + next.size;
            });
        num_tasks_  = files.files.size();
        num_slices_ = files.files.size();
        filebatch_  = true;

        LOG_I("Batch ", ui_.batch_id, " (", ui_.desc, ") created");
        LOG_D("Batch has ", num_tasks_, " tasks");

        statesets_[(int)states::Queued] = num_tasks_;
    }

    BatchState(std::size_t id, const ui::GroupListDownload& list) : BatchState(id)
    {
        ui_.account  = list.account;
        ui_.desc     = list.desc;
        num_tasks_   = 1;
        num_slices_  = 1;

        LOG_I("Batch ", ui_.batch_id, " (", ui_.desc, ") created");
        LOG_D("Batch has 1 task");

        statesets_[(int)states::Queued] = num_tasks_;
    }

    BatchState(std::size_t id, const ui::HeaderDownload& download) : BatchState(id)
    {
        ui_.account = download.account;
        ui_.desc    = download.desc;
        num_tasks_  = 1;
        num_slices_ = 1;

        LOG_I("Batch ", ui_.batch_id, " (", ui_.desc, ") created");
        LOG_D("Batch has 1 tasks");
        statesets_[(int)states::Queued] = num_tasks_;
    }

    BatchState(const data::Batch& data) : BatchState(0)
    {
        ui_.batch_id   = data.batch_id();
        ui_.task_id    = data.batch_id();
        ui_.account    = data.account_id();
        ui_.desc       = data.desc();
        ui_.path       = data.path();
        ui_.size       = data.byte_size();
        num_slices_    = data.num_slices();
        num_tasks_     = data.num_tasks();
        filebatch_     = true;

        statesets_[(int)states::Queued] = num_tasks_;
    }

   ~BatchState()
    {
        LOG_I("Batch ", ui_.batch_id, " deleted");
    }

    void serialize(data::TaskList& list)
    {
        if (ui_.state == states::Complete || ui_.state == states::Error)
            return;
        if (!filebatch_)
            return;

        auto* data = list.add_batch();
        data->set_batch_id(ui_.batch_id);
        data->set_account_id(ui_.account);
        data->set_desc(ui_.desc);
        data->set_byte_size(ui_.size);
        data->set_path(ui_.path);
        data->set_num_tasks(num_tasks_);
        data->set_num_slices(num_slices_);
    }

    void lock(Engine::State& state)
    {
        LOG_D("Batch lock ", ui_.batch_id);

        for (auto& task : state.tasks)
        {
            if (task->bid() != ui_.batch_id)
                continue;

            task->lock(state);
        }
    }

    void unlock(Engine::State& state)
    {
        LOG_D("Batch unlock ", ui_.batch_id);

        for (auto& task : state.tasks)
        {
            if (task->bid() != ui_.batch_id)
                continue;

            task->unlock(state);
        }
    }

    void pause(Engine::State& state)
    {
        LOG_D("Batch pause ", ui_.batch_id);

        for (auto& task : state.tasks)
        {
            if (task->bid() != ui_.batch_id)
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

    void resume(Engine::State& state)
    {
        LOG_D("Batch resume ", ui_.batch_id);

        for (auto& task : state.tasks)
        {
            if (task->bid() != ui_.batch_id)
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

    void update(Engine::State& state, ui::TaskDesc& ui)
    {
        ui = ui_;
        ui.etatime = 0;
        if (ui_.state == states::Waiting || ui_.state == states::Active)
        {
            if (ui_.runtime >= 10)
            {
                const auto complete = ui.completion;
                const auto remaining = 100.0 - complete;
                const auto timespent = ui.runtime;
                const auto time_per_percent = timespent / complete;
                const auto time_remains = remaining * time_per_percent;
                ui.etatime = (std::uint32_t)time_remains;
            }
        }
    }

    void kill(Engine::State& state)
    {
        LOG_D("Batch kill ", ui_.batch_id);

        // partition tasks so that the tasks that belong to this batch
        // are at the end of the the task list.
        auto it = std::stable_partition(std::begin(state.tasks), std::end(state.tasks),
            [&](const std::unique_ptr<TaskState>& t) {
                return t->bid() != ui_.batch_id;
            });

        for (auto i=it; i != std::end(state.tasks); ++i)
            (*i)->kill(state);

        state.tasks.erase(it, std::end(state.tasks));
    }

    bool kill(Engine::State& state, const TaskState& t)
    {
        leave_state(t, t.state());
        num_tasks_--;
        update(state);

        return num_tasks_ == 0;
    }

    void update(Engine::State& state, const TaskState& t, const TaskState::transition& s)
    {
        ui_.error |= t.errors();

        leave_state(t, s.previous);
        enter_state(t, s.current);
        update(state);

        if (!filebatch_)
            return;

        if (s.current == states::Complete || s.current == states::Error)
        {
            if (ui_.state == states::Complete)
            {
                ui::FileBatchResult result;
                result.account = ui_.account;
                result.path    = ui_.path;
                result.desc    = ui_.desc;
                state.on_batch_callback(result);
            }
        }
    }

    void tick(Engine::State& state, const std::chrono::milliseconds& elapsed)
    {
        if (ui_.state == states::Active)
            ui_.runtime++;

        double total = 0.0;

        const double slice = 1.0 / (double)num_slices_;
        for (auto it = std::begin(state.tasks); it != std::end(state.tasks); ++it)
        {
            const auto& task = *it;
            if (task->bid() != ui_.batch_id)
                continue;
            double done = task->done() / 100.0;
            total += done * slice;
        }
        ui_.completion  = total * 100.0;
        ui_.completion += (num_slices_ - num_tasks_) * slice * 100.0;
    }

    std::size_t id() const
    { return ui_.batch_id; }

private:
    void enter_state(const TaskState& t, states s)
    {
        statesets_[(int)s]++;
    }
    void leave_state(const TaskState& t, states s)
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
    bool is_full_set(states a, states b)
    {
        const auto num_a = statesets_[(int)a];
        const auto num_b = statesets_[(int)b];
        return num_a + num_b == num_tasks_;
    }

    void update(Engine::State& state)
    {
        const std::size_t num_states = std::accumulate(std::begin(statesets_),
            std::end(statesets_), 0);
        assert(num_states == num_tasks_);
        (void)num_states;

        if (!is_empty_set(states::Active))
            goto_state(state, states::Active);
        else if (!is_empty_set(states::Waiting))
            goto_state(state, states::Waiting);
        else if (is_full_set(states::Crunching))
            goto_state(state, states::Crunching);

        if (!is_empty_set(states::Paused))
            goto_state(state, states::Paused);
        else if (is_full_set(states::Complete, states::Error))
        {
            if (is_full_set(states::Error))
                goto_state(state, states::Error);
            else goto_state(state, states::Complete);
        }
        else if (is_full_set(states::Error))
            goto_state(state, states::Error);
        else if (is_full_set(states::Queued))
            goto_state(state, states::Queued);
    }

    void goto_state(Engine::State& state, states new_state)
    {
        ui_.state = new_state;
        switch (new_state)
        {
            case states::Complete:
                ui_.completion = 100.0;
                ui_.etatime    = 0;
                break;

            default:
            break;
        }

        LOG_D("Batch has ", num_tasks_, " tasks");
        LOG_D("Batch tasks"
              " Queued: ", statesets_[(int)states::Queued],
              " Waiting: ", statesets_[(int)states::Waiting],
              " Active: ", statesets_[(int)states::Active],
              " Crunching: ", statesets_[(int)states::Crunching],
              " Paused: ", statesets_[(int)states::Paused],
              " Complete: ", statesets_[(int)states::Complete],
              " Errored: ", statesets_[(int)states::Error]);
        LOG_D("Batch ", ui_.batch_id, " => ", str(new_state));
    }
private:
    ui::TaskDesc ui_;
private:
    std::size_t num_slices_;
    std::size_t num_tasks_;
    std::size_t statesets_[7];
    bool filebatch_;
};

Engine::BatchState* Engine::State::find_batch(std::size_t id)
{
    auto it = std::find_if(std::begin(batches), std::end(batches),
        [&](const std::unique_ptr<BatchState>& b) {
            return b->id() == id;
        });
    if (it == std::end(batches))
        return nullptr;

    return (*it).get();
}

void Engine::State::execute()
{
    if (!started)
        return;

    for (auto it = std::begin(tasks); it != std::end(tasks); ++it)
    {
        auto& task  = *it;
        if (!task->eligible_for_run())
            continue;

        std::size_t num_conns = std::count_if(std::begin(conns), std::end(conns),
            [&](const std::unique_ptr<ConnState>& c) {
                return c->account() == task->account();
            });

        const auto& acc = find_account(task->account());
        for (auto i=num_conns; i<acc.connections; ++i)
            conns.emplace_back(new ConnState(*this, acc.id, oid++));

        while (task->eligible_for_run())
        {
            auto it = std::find_if(std::begin(conns), std::end(conns),
                [&](const std::unique_ptr<ConnState>& c) {
                    return c->is_ready() && (c->account() == task->account());
                });
            if (it == std::end(conns))
                break;

            const auto transition = task->run(*this);
            if (transition)
            {
                auto* batch = find_batch(task->bid());
                batch->update(*this, *task, transition);
            }
        }
    }
}

void Engine::State::enqueue(const TaskState& t, std::shared_ptr<cmdlist> cmd)
{
    cmd->set_conn(0);
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
        // const auto num_acc   = cmd->account();
        // const auto num_tasks = std::count_if(std::begin(tasks), std::end(tasks),
        //     [=](const std::unique_ptr<engine::task> task) {
        //         return task->account() == num_acc;
        //     };
        // if (num_tasks < num_conns)
        //     return;

        const auto id = oid++;
        conns.emplace_back(new ConnState(*this, acc.id, id));
    }

}


Engine::Engine(std::unique_ptr<Factory> factory) : state_(new State)
{
    state_->factory = std::move(factory);
}

Engine::Engine() : state_(new State)
{
    class DefaultFactory : public Factory
    {
    public:
        std::unique_ptr<task> AllocateTask(const ui::FileDownload& file) override
        {
            return std::make_unique<download>(file.groups, file.articles, file.path, file.name);
        }

        std::unique_ptr<task> AllocateTask(const ui::HeaderDownload& download) override
        {
            return std::make_unique<update>(download.path, download.group);
        }

        std::unique_ptr<task> AllocateTask(const ui::GroupListDownload& list) override
        {
            return std::make_unique<listing>();
        }
        std::unique_ptr<connection> AllocateConnection()
        {
            return std::make_unique<connection>();
        }
        std::vector<std::unique_ptr<ui::Result>> MakeResult(const task& task, const ui::TaskDesc& desc) const override
        {
            std::vector<std::unique_ptr<ui::Result>> ret;
            if (const auto* ptr = dynamic_cast<const download*>(&task))
            {
                const auto& files = ptr->files();
                for (const auto& file : files)
                {
                    auto result = std::make_unique<ui::FileResult>();
                    result->account = desc.account;
                    result->damaged = desc.error.any_bit();
                    result->binary  = file->is_binary();
                    result->name    = file->filename();
                    result->path    = file->filepath();
                    result->size    = file->size();
                    ret.push_back(std::move(result));
                }
            }
            else if (const auto* ptr = dynamic_cast<const listing*>(&task))
            {
                auto result = std::make_unique<ui::GroupListResult>();
                result->account = desc.account;
                result->desc    = desc.desc;

                const auto& groups = ptr->group_list();
                for (const auto& g : groups)
                {
                    ui::GroupListResult::Newsgroup group;
                    group.name  = g.name;
                    group.first = g.first;
                    group.last  = g.last;
                    group.size  = g.size;
                    result->groups.push_back(group);
                }
                ret.push_back(std::move(result));
            }
            else if (const auto* ptr = dynamic_cast<const update*>(&task))
            {
                auto result = std::make_unique<ui::HeaderResult>();
                result->account = desc.account;
                result->desc    = desc.desc;
                result->group   = ptr->group();
                result->path    = ptr->path();
                result->num_local_articles  = ptr->num_local_articles();
                result->num_remote_articles = ptr->num_remote_articles();
            }
            return ret;
        }
    };
    state_->factory = std::make_unique<DefaultFactory>();
}

Engine::~Engine()
{
    assert(state_->num_pending_actions == 0);
    assert(state_->conns.empty());
    assert(state_->started == false);

    state_->threads->shutdown();
    state_->threads.reset();
}

void Engine::TryAccount(const ui::Account& account)
{
    const auto id = state_->oid++;

    state_->current_connection_test.reset(new ConnTestState(account, id, *state_));
}

void Engine::SetAccount(const ui::Account& acc)
{
    auto it = std::find_if(std::begin(state_->accounts), std::end(state_->accounts),
        [&](const ui::Account& a) {
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
        [&](const std::unique_ptr<ConnState>& c)
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
        [&](const std::unique_ptr<ConnState>& c) {
            return c->account() == acc.id;
        });

    if (num_conns < acc.connections)
    {
        if (!state_->started)
            return;

        for (auto i = num_conns; i<acc.connections; ++i)
        {
            const auto cid = state_->oid++;
            state_->conns.emplace_back(new ConnState(*state_, acc.id, cid));
        }
    }
    else if (num_conns > acc.connections)
    {
        state_->conns.resize(acc.connections);
    }
}

void Engine::DelAccount(std::size_t id)
{
    auto end = std::remove_if(std::begin(state_->conns), std::end(state_->conns),
        [&](const std::unique_ptr<ConnState>& c) {
            return c->account() == id;
        });

    state_->conns.erase(end, std::end(state_->conns));

    auto it = std::find_if(std::begin(state_->accounts), std::end(state_->accounts),
        [&](const ui::Account& a) {
            return a.id == id;
        });
    ASSERT(it != std::end(state_->accounts));

    state_->accounts.erase(it);
}

void Engine::SetFillAccount(std::size_t id)
{
    state_->fill_account = id;
}

Engine::TaskId Engine::DownloadFiles(const ui::FileBatchDownload& batch, bool priority)
{
    const auto batchid = state_->oid++;
    std::unique_ptr<BatchState> b(new BatchState(batchid, batch));

    settings s;
    s.discard_text_content = state_->discard_text;
    s.overwrite_existing_files = state_->overwrite_existing;

    for (auto& file : batch.files)
    {
        const auto taskid = state_->oid++;

        assert(file.path == batch.path);

        std::unique_ptr<TaskState> job(new TaskState(*state_, taskid, std::move(file)));
        job->configure(s);
        job->set_account(batch.account);
        job->set_batch(batchid);

        assert(job->is_valid());

        if (priority)
            state_->tasks.push_front(std::move(job));
        else state_->tasks.push_back(std::move(job));

        state_->bytes_queued += file.size;
        state_->num_pending_tasks++;
    }
    if (priority)
        state_->batches.push_front(std::move(b));
    else state_->batches.push_back(std::move(b));
    state_->execute();
    return batchid;
}

Engine::TaskId Engine::DownloadListing(const ui::GroupListDownload& list)
{
    const auto batchid = state_->oid++;
    std::unique_ptr<BatchState> batch(new BatchState(batchid, list));

    const auto taskid = state_->oid++;
    std::unique_ptr<TaskState> job(new TaskState(*state_, taskid, list));
    job->set_account(list.account);
    job->set_batch(batchid);

    assert(job->is_valid());

    state_->tasks.push_back(std::move(job));
    state_->batches.push_back(std::move(batch));
    state_->num_pending_tasks++;
    state_->execute();
    return batchid;
}

Engine::TaskId Engine::DownloadHeaders(const ui::HeaderDownload& download)
{
    const auto batchid = state_->oid++;
    std::unique_ptr<BatchState> batch(new BatchState(batchid, download));

    const auto taskid = state_->oid++;
    std::unique_ptr<TaskState> task(new TaskState(*state_, taskid, download));
    task->set_account(download.account);
    task->set_batch(batchid);

    assert(task->is_valid());

    state_->tasks.push_back(std::move(task));
    state_->batches.push_back(std::move(batch));
    state_->num_pending_tasks++;
    state_->execute();
    return batchid;
}

bool Engine::Pump()
{
// #ifdef NEWSFLASH_DEBUG
//     {
//         std::lock_guard<std::mutex> lock(state_->mutex);
//         std::size_t num_actions = state_->actions.size();
//         if (num_actions == 0)
//             return state_->num_pending_actions;
//         LOG_D("Pumping all ", num_actions, " currently completed actions");
//     }
// #endif
    state_->quit_pump_loop = false;

    set_thread_log(state_->logger.get());

    for (;;)
    {
        if (state_->quit_pump_loop)
            break;

        std::unique_ptr<action> action = state_->get_action();
        if (!action)
            break;

        if (auto* e = dynamic_cast<class connection::execute*>(action.get()))
        {
            auto cmds  = e->get_cmdlist();
            auto tid   = e->get_tid();
            auto bytes = e->get_content_transferred(); //e->get_bytes_transferred();
            if (cmds->cmdtype() == cmdlist::type::body)
            {
                if (state_->on_quota_callback && bytes)
                    state_->on_quota_callback(bytes, cmds->account());
            }

            LOG_D("Cmdlist ", cmds->id(), " executed");
            LOG_D("Cmdlist ", cmds->id(), " belongs to task ", cmds->task());
            LOG_D("Cmdlist ", cmds->id(), " goodbit: ", cmds->is_good());
            LOG_D("Cmdlist ", cmds->id(), " cancelbit: ", cmds->is_canceled());

            #ifdef NEWSFLASH_DEBUG
            if (std::getenv("NEWSFLASH_DUMP_DATA"))
            {
                const auto& buffers  = cmds->get_buffers();
                const auto& commands = cmds->get_commands();
                for (std::size_t i=0; i<buffers.size(); ++i)
                {
                    if (i >= commands.size())
                        break;
                    const auto& buff = buffers[i];
                    std::ofstream out;
                    std::stringstream ss;
                    ss << "/tmp/Newsflash/" << commands[i] << ".txt";
                    std::string file;
                    ss >> file;
                    out.open(file, std::ios::binary | std::ios::app);
                    if (buff.content_status() == buffer::status::success)
                        out.write(buff.content(), buff.content_length()-3);
                    out.flush();
                }
            }
            #endif

            auto tit  = std::find_if(std::begin(state_->tasks), std::end(state_->tasks),
                [&](const std::unique_ptr<TaskState>& t) {
                    return t->tid() == tid;
                });
            if (tit != std::end(state_->tasks))
            {
                auto& task = *tit;

                if (e->has_exception())
                {
                    LOG_E("Action ", e->get_id(), " (", e->describe(), " ) has an exception. Cmdlist ", cmds->id(), " not ready.");
                    state_->enqueue(*task, cmds);
                }
                else
                {
                    auto it = std::find(std::begin(state_->cmds), std::end(state_->cmds), cmds);
                    if (it != std::end(state_->cmds))
                        state_->cmds.erase(it);

                    const auto transition = task->complete(*state_, cmds);
                    if (transition)
                    {
                        auto* batch = state_->find_batch(task->bid());
                        batch->update(*state_, *task, transition);
                    }
                }
            }
            state_->bytes_downloaded += bytes;
        }
        else if (auto* w = dynamic_cast<class datafile::write*>(action.get()))
        {
            auto bytes = w->get_write_size();
            if (!w->has_exception())
                state_->bytes_written += bytes;
        }

        const auto id = action->get_owner();
        auto it = std::find_if(std::begin(state_->conns), std::end(state_->conns),
            [&](const std::unique_ptr<ConnState>& c) {
                return c->id() == id;
            });
        if (it != std::end(state_->conns))
        {
            auto& conn = *it;
            conn->on_action(*state_, std::move(action));
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
                        [&](const std::unique_ptr<TaskState>& t) {
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
                        [&](const std::unique_ptr<TaskState>& t) {
                            return t->account() == conn->account() &&
                                   t->eligible_for_run();
                               });
                    if (it != std::end(state_->tasks))
                    {
                        auto& task = *it;
                        const auto transition = task->run(*state_);
                        if (transition)
                        {
                            auto* batch = state_->find_batch(task->bid());
                            batch->update(*state_, *task, transition);
                        }
                    }
                }
            }
        }
        else if (state_->current_connection_test &&
            state_->current_connection_test->id() == id)
        {
            state_->current_connection_test->on_action(*state_, std::move(action));
            if (state_->current_connection_test->is_finished())
                state_->current_connection_test.reset();
        }
        else
        {
            for (auto& task : state_->tasks)
            {
                if (task->tid() != id)
                    continue;

                const auto transition = task->on_action(*state_, std::move(action));
                if (transition)
                {
                    auto* batch = state_->find_batch(task->bid());
                    batch->update(*state_, *task, transition);
                }
                // if (task->eligible_for_run())
                // {
                //     const auto transition = task->run(*state_);
                //     if (transition)
                //     {
                //         auto& batch = state_->find_batch(task->bid());
                //         batch.update(*state_, *task, transition);
                //     }
                // }
                break;
            }
            state_->execute();
        }
        state_->num_pending_actions--;
    }

    LOG_FLUSH();

    return state_->num_pending_actions != 0;
}

void Engine::Tick()
{
    if (state_->repartition_task_list)
    {
        auto tit = std::begin(state_->tasks);

        for (auto& batch : state_->batches)
        {
            tit = std::stable_partition(tit, std::end(state_->tasks),
                [&](const std::unique_ptr<TaskState>& t) {
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


void Engine::Start(std::string logs)
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

void Engine::Stop()
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

void Engine::SaveSession(const std::string& file)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

#if defined(WINDOWS_OS)
    // msvc specific extension..
    std::ofstream out(utf8::decode(file), std::ios::out | std::ios::binary | std::ios::trunc);
#elif defined(LINUX_OS)
    std::ofstream out(file, std::ios::out | std::ios::binary | std::ios::trunc);
#endif
    if (!out.is_open())
        throw std::system_error(errno, std::generic_category(), "failed to sdata in: " + file);

    data::TaskList list;

    for (const auto& acc : state_->accounts)
    {
        auto* pa = list.add_account();
        pa->set_id(acc.id);
        pa->set_name(acc.name);
        pa->set_user(acc.username);
        pa->set_pass(acc.password);
        pa->set_secure_host(acc.secure_host);
        pa->set_secure_port(acc.secure_port);
        pa->set_general_host(acc.general_host);
        pa->set_general_port(acc.general_port);
        pa->set_max_connections(acc.connections);
        pa->set_enable_secure_server(acc.enable_secure_server);
        pa->set_enable_general_server(acc.enable_general_server);
        pa->set_enable_pipelining(acc.enable_pipelining);
        pa->set_enable_compression(acc.enable_compression);
    }

    for (const auto& batch : state_->batches)
    {
        batch->serialize(list);
    }

    for (const auto& task : state_->tasks)
    {
        task->serialize(list);
    }

    list.set_current_id(state_->oid);
    list.set_bytes_queued(state_->bytes_queued);
    list.set_bytes_ready(state_->bytes_ready);

    if (!list.SerializeToOstream(&out))
        throw std::runtime_error("engine serialize to stream failed");
}

void Engine::LoadSession(const std::string& file)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

#if defined(WINDOWS_OS)
    // msvc extension
    std::ifstream in(utf8::decode(file), std::ios::in | std::ios::binary);
#elif defined(LINUX_OS)
    std::ifstream in(file, std::ios::in | std::ios::binary);
#endif
    if (!in.is_open())
        throw std::system_error(errno, std::generic_category(), "failed to ldata from: " + file);

    data::TaskList list;
    if (!list.ParseFromIstream(&in))
        throw std::runtime_error("engine parse from stream failed");

    for (int i=0; i<list.account_size(); ++i)
    {
        const auto& p = list.account(i);
        ui::Account a;
        a.id                    = p.id();
        a.name                  = p.name();
        a.username              = p.user();
        a.password              = p.pass();
        a.secure_host           = p.secure_host();
        a.secure_port           = p.secure_port();
        a.general_host          = p.general_host();
        a.general_port          = p.general_port();
        a.connections           = p.max_connections();
        a.enable_secure_server  = p.enable_secure_server();
        a.enable_general_server = p.enable_general_server();
        a.enable_pipelining     = p.enable_pipelining();
        a.enable_compression    = p.enable_compression();
        SetAccount(a);
    }

    for (int i=0; i<list.batch_size(); ++i)
    {
        const auto& data = list.batch(i);
        std::unique_ptr<BatchState> batch(new BatchState(data));
        state_->batches.push_back(std::move(batch));
    }

    settings s;
    s.discard_text_content = state_->discard_text;
    s.overwrite_existing_files = state_->overwrite_existing;

    for (int i=0; i<list.download_size();++i)
    {
        const auto& data = list.download(i);
        std::unique_ptr<TaskState> task(new TaskState(*state_, data));
        task->configure(s);

        assert(task->is_valid());

        state_->tasks.push_back(std::move(task));
    }

    state_->oid = list.current_id();
    state_->bytes_queued = list.bytes_queued();
    state_->bytes_ready  = list.bytes_ready();
    if (list.download_size() == 0)
        state_->oid = 1;
}


void Engine::SetErrorCallback(on_error error_callback)
{
    state_->on_error_callback = std::move(error_callback);
}

void Engine::SetFileCallback(on_file file_callback)
{
    state_->on_file_callback = std::move(file_callback);
}

void Engine::SetNotifyCallback(on_async_notify notify_callback)
{
    state_->on_notify_callback = std::move(notify_callback);
}

void Engine::SetHeaderDataCallback(on_header_data callback)
{
    state_->on_header_data_callback = std::move(callback);
}

void Engine::SetHeaderInfoCallback(on_header_info callback)
{
    state_->on_header_info_callback = std::move(callback);
}

void Engine::SetBatchCallback(on_batch batch_callback)
{
    state_->on_batch_callback = std::move(batch_callback);
}

void Engine::SetListCallback(on_list list_callback)
{
    state_->on_list_callback = std::move(list_callback);
}

void Engine::SetTaskCallback(on_task task_callback)
{
    state_->on_task_callback = std::move(task_callback);
}

void Engine::SetUpdateCallback(on_update update_callback)
{
    state_->on_update_callback = std::move(update_callback);
}

void Engine::SetFinishCallback(on_finish callback)
{
    state_->on_finish_callback = std::move(callback);
}

void Engine::SetQuotaCallback(on_quota callback)
{
    state_->on_quota_callback = std::move(callback);
}

void Engine::SetTestCallback(on_conn_test callback)
{
    state_->on_test_callback = std::move(callback);
}

void Engine::SetTestLogCallback(on_conn_test_log callback)
{
    state_->on_test_log_callback = std::move(callback);
}

void Engine::SetOverwriteExistingFiles(bool on_off)
{
    state_->overwrite_existing = on_off;

    settings s;
    s.overwrite_existing_files = on_off;
    s.discard_text_content = state_->discard_text;

    for (auto& t: state_->tasks)
        t->configure(s);
}

void Engine::SetDiscardTextContent(bool on_off)
{
    state_->discard_text = on_off;

    settings s;
    s.discard_text_content = on_off;
    s.overwrite_existing_files = state_->overwrite_existing;

    for (auto& t : state_->tasks)
        t->configure(s);
}

void Engine::SetPreferSecure(bool on_off)
{
    state_->prefer_secure = on_off;
}

void Engine::SetEnableThrottle(bool on_off)
{
    state_->ratecontrol.enable(on_off);
    LOG_D("Throttle is now ", on_off);
}

void Engine::SetThrottleValue(unsigned value)
{
    state_->ratecontrol.set_quota(value);
    LOG_D("Throttle value ", value, " bytes per second.");
}


void Engine::SetGroupItems(bool on_off)
{
    state_->group_items = on_off;

    if (state_->group_items)
    {
        auto tit = std::begin(state_->tasks);

        for (auto& batch : state_->batches)
        {
            tit = std::stable_partition(tit, std::end(state_->tasks),
                [&](const std::unique_ptr<TaskState>& t) {
                    return t->bid() == batch->id();
                });
        }

        state_->repartition_task_list = false;
    }
}

bool Engine::GetGroupItems() const
{
    return state_->group_items;
}

bool Engine::GetOverwriteExistingFiles() const
{
    return state_->overwrite_existing;
}

bool Engine::GetDiscardTextContent() const
{
    return state_->discard_text;
}

bool Engine::GetPreferSecure() const
{
    return state_->prefer_secure;
}

bool Engine::GetEnableThrottle() const
{
    return state_->ratecontrol.is_enabled();
}

unsigned Engine::GetThrottleValue() const
{
    return state_->ratecontrol.get_quota();
}

std::uint64_t Engine::GetCurrentQueueSize() const
{
    return state_->bytes_queued;
}

std::uint64_t Engine::GetBytesReady() const
{
    return state_->bytes_ready;
}

std::uint64_t Engine::GetTotalBytesWritten() const
{
    return state_->bytes_written;
}

std::uint64_t Engine::GetTotalBytesDownloaded() const
{
    return state_->bytes_downloaded;
}

std::string Engine::GetLogfileName() const
{
    return fs::joinpath(state_->logpath, "engine.log");
}


bool Engine::IsStarted() const
{
    return state_->started;
}

void Engine::GetTasks(std::deque<ui::TaskDesc>* tasklist)
{
    if (state_->group_items)
    {
        const auto& batches = state_->batches;

        tasklist->resize(batches.size());

        for (std::size_t i=0; i<batches.size(); ++i)
            batches[i]->update(*state_, (*tasklist)[i]);
    }
    else
    {
        const auto& tasks = state_->tasks;

        tasklist->resize(tasks.size());

        for (std::size_t i=0; i<tasks.size(); ++i)
            tasks[i]->update(*state_, (*tasklist)[i]);
    }
}

void Engine::GetConns(std::deque<ui::Connection>* connlist)
{
    const auto& conns = state_->conns;

    connlist->resize(conns.size());

    for (std::size_t i=0; i<conns.size(); ++i)
    {
        conns[i]->update((*connlist)[i]);
    }
}

void Engine::KillConnection(std::size_t i)
{
    LOG_D("Kill connection ", i);

    ASSERT(i < state_->conns.size());

    auto it = state_->conns.begin();
    it += i;
    (*it)->stop(*state_);
    state_->conns.erase(it);
}

void Engine::CloneConnection(std::size_t i)
{
    LOG_D("Clone connection ", i);

    ASSERT(i < state_->conns.size());

    const auto cid = state_->oid++;

    auto& dna  = state_->conns[i];
    auto dolly = std::unique_ptr<ConnState>(new ConnState(*state_, cid, *dna));
    state_->conns.push_back(std::move(dolly));
}

Engine::TaskId Engine::KillTask(std::size_t i)
{
    TaskId killed = 0;

    if (state_->group_items)
    {
        ASSERT(i < state_->batches.size());

        auto it = state_->batches.begin();
        it += i;
        killed = (*it)->id();
        (*it)->kill(*state_);
        state_->batches.erase(it);
    }
    else
    {
        ASSERT(i < state_->tasks.size());

        auto tit = state_->tasks.begin();
        tit += i;

        auto bit = std::find_if(std::begin(state_->batches), std::end(state_->batches),
            [&](const std::unique_ptr<BatchState>& b) {
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

    return killed;
}

Engine::TaskId Engine::PauseTask(std::size_t index)
{
    TaskId paused = 0;

    if (state_->group_items)
    {
        ASSERT(index < state_->batches.size());

        state_->batches[index]->pause(*state_);

        paused = state_->batches[index]->id();
    }
    else
    {
        ASSERT(index < state_->tasks.size());

        auto& task  = state_->tasks[index];

        const auto transition = task->pause(*state_);
        if (transition)
        {
            auto* batch = state_->find_batch(task->bid());
            batch->update(*state_, *task, transition);
        }
    }

    return paused;
}

Engine::TaskId Engine::ResumeTask(std::size_t index)
{
    TaskId resumed = 0;

    if (state_->group_items)
    {
        assert(index < state_->batches.size());

        state_->batches[index]->resume(*state_);

        resumed = state_->batches[index]->id();
    }
    else
    {

        assert(index < state_->tasks.size());

        auto& task  = state_->tasks[index];

        const auto transition = task->resume(*state_);
        if (transition)
        {
            auto* batch = state_->find_batch(task->bid());
            batch->update(*state_, *task, transition);
        }

    }
    state_->execute();

    return resumed;
}

void Engine::MoveTaskUp(std::size_t index)
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

void Engine::MoveTaskDown(std::size_t index)
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

bool Engine::KillTaskById(Engine::TaskId id)
{
    auto it = std::find_if(std::begin(state_->batches), std::end(state_->batches),
        [&](const std::unique_ptr<BatchState>& b) {
            return b->id() == id;
        });

    // it's possible that the task is automatically killed
    // for example in the case where it completes with an error
    // not all the UI handles these cases properly and then incorrectly
    // call us to kill the action
    //ASSERT(it != std::end(state_->batches));
    if (it == std::end(state_->batches))
        return false;

    (*it)->kill(*state_);

    state_->batches.erase(it);
    return true;
}

bool Engine::LockTaskById(TaskId id)
{
    auto* batch = state_->find_batch(id);
    if (!batch)
        return false;

    batch->lock(*state_);
    return true;
}

bool Engine::UnlockTaskById(TaskId id)
{
    auto* batch = state_->find_batch(id);
    if (!batch)
        return false;

    batch->unlock(*state_);
    return true;
}

std::size_t Engine::GetNumTasks() const
{
    return state_->tasks.size();
}

std::size_t Engine::GetNumPendingTasks() const
{
    return state_->num_pending_tasks;
}


void initialize()
{
    // msvs doesn't implement magic static untill msvs2015
    // details here;
    // https://msdn.microsoft.com/en-us/library/hh567368.aspx
    nntp::thread_safe_initialize();

    openssl_init();

    identify_encoding("", 0);
}

} // newsflash
