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
#include "filetype.h"
#include "logging.h"
#include "utf8.h"
#include "threadpool.h"
#include "cmdlist.h"
#include "datafile.h"
#include "listing.h"
#include "update.h"
#include "throttle.h"
#include "sslcontext.h"
#include "encoding.h"
#include "nntp.h"
#include "utility.h"

namespace newsflash
{

// default factory that is used to construct the real task and connection
// objects when no other factory is given to the engine.
// Moved here instead of being the Engine's ctor since msvs2013 barfs
// at this construct.
class DefaultFactory : public Engine::Factory
{
public:
    DefaultFactory(const std::string& logpath) : logpath_(logpath)
    {}

    std::unique_ptr<Task> AllocateTask(const ui::FileDownload& file) override
    {
        return std::make_unique<Download>(file.groups, file.articles, file.path, file.desc);
    }

    std::unique_ptr<Task> AllocateTask(const ui::HeaderDownload& download) override
    {
        return std::make_unique<Update>(download.path, download.group);
    }

    std::unique_ptr<Task> AllocateTask(const ui::GroupListDownload& list) override
    {
        return std::make_unique<Listing>();
    }
    std::unique_ptr<Task> AllocateTask(std::size_t type) override
    {
        // todo: fix this.
        ASSERT(type == 1);

        return std::make_unique<Download>();
    }

    std::unique_ptr<Connection> AllocateConnection(const ui::Account& acc)
    {
        return std::make_unique<ConnectionImpl>();
    }
    std::unique_ptr<ui::Result> MakeResult(const Task& task, const ui::TaskDesc& desc) const override
    {
        std::unique_ptr<ui::Result> ret;
        if (const auto* ptr = dynamic_cast<const Download*>(&task))
        {
            auto result = std::make_unique<ui::FileResult>();
            result->account = desc.account;
            result->desc    = desc.desc;

            for (size_t i=0; i<ptr->GetNumFiles(); ++i)
            {
                const auto* file = ptr->GetFile(i);
                ui::FileResult::File f;
                f.damaged = desc.error.any_bit();
                f.binary  = file->IsBinary();
                f.name    = file->GetFileName();
                f.path    = file->GetFilePath();
                f.size    = file->GetFileSize();
                result->files.push_back(std::move(f));
            }
            ret = std::move(result);
        }
        else if (const auto* ptr = dynamic_cast<const Listing*>(&task))
        {
            auto result = std::make_unique<ui::GroupListResult>();
            result->account = desc.account;
            result->desc    = desc.desc;

            for (size_t i=0; i<ptr->NumGroups(); ++i)
            {
                const auto& g = ptr->GetGroup(i);
                ui::GroupListResult::Newsgroup group;
                group.name  = g.name;
                group.first = g.first;
                group.last  = g.last;
                group.size  = g.size;
                result->groups.push_back(group);
            }
            ret = std::move(result);
        }
        else if (const auto* ptr = dynamic_cast<const Update*>(&task))
        {
            auto result = std::make_unique<ui::HeaderResult>();
            result->account = desc.account;
            result->desc    = desc.desc;
            result->group   = ptr->group();
            result->path    = ptr->path();
            result->num_local_articles  = ptr->num_local_articles();
            result->num_remote_articles = ptr->num_remote_articles();
            result->local_last = ptr->get_local_last();
            result->local_first = ptr->get_local_first();
            result->remote_first = ptr->get_remote_first();
            result->remote_last = ptr->get_remote_last();
            ret = std::move(result);
        }
        return ret;
    }
    std::unique_ptr<Logger> AllocateEngineLogger()
    {
        const auto& logfile = fs::joinpath(logpath_, "engine.log");

        std::unique_ptr<Logger> logger(new FileLogger(logfile, true));
        return logger;
    }
    std::unique_ptr<Logger> AllocateConnectionLogger()
    {
        const auto& name = str("connection", lognum_, ".log");
        const auto& file = fs::joinpath(logpath_, name);
        lognum_++;
        std::unique_ptr<Logger> logger(new FileLogger(file, true));
        return logger;
    }
private:
    std::string logpath_;
    std::size_t lognum_ = 1;
};

#define CASE(x) case x: return #x

const char* str(Buffer::Status status)
{
    using state = Buffer::Status;
    switch (status)
    {
        CASE(state::None);
        CASE(state::Success);
        CASE(state::Unavailable);
        CASE(state::Dmca);
        CASE(state::Error);
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
    std::list<std::shared_ptr<CmdList>> cmds;
    std::uint64_t bytes_downloaded = 0;
    std::uint64_t bytes_queued = 0;
    std::uint64_t bytes_ready = 0;
    std::uint64_t bytes_written = 0;
    std::size_t oid = 1; // object id for tasks/connections
    std::size_t fill_account = 0;

    std::unique_ptr<Engine::Factory> factory;

    std::unique_ptr<Logger> logger;
    std::unique_ptr<ConnTestState> current_connection_test;

    std::mutex mutex;
    std::queue<std::unique_ptr<action>> actions;
    std::unique_ptr<ThreadPool> threads;
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
    Engine::on_header_update on_header_update_callback;
    Engine::on_listing_update on_listing_update_callback;
    Engine::on_async_notify on_notify_callback;
    Engine::on_finish on_finish_callback;
    Engine::on_quota on_quota_callback;
    Engine::on_conn_test on_test_callback;
    Engine::on_conn_test_log on_test_log_callback;

    throttle ratecontrol;

    State(bool enable_single_thread_debug)
    {
        ratecontrol.set_quota(std::numeric_limits<std::size_t>::max());
        if (enable_single_thread_debug)
        {
            threads.reset(new ThreadPool(0));
            const auto AddToPool = true;
            const auto AddAsPrivate = true;
            threads->AddMainThread(AddToPool, AddAsPrivate);
        }
        else
        {
            const auto max_pooled_threads  = std::size_t(4);
            threads.reset(new ThreadPool(max_pooled_threads));
        }
        threads->SetCallback(
            [&](action* a)
            {
                std::lock_guard<std::mutex> lock(mutex);
                actions.emplace(a);
                if (on_notify_callback)
                    on_notify_callback();
            }
        );
    }

   ~State()
    {
        tasks.clear();
        conns.clear();
        batches.clear();
        SetThreadLog(nullptr);
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

            threads->Submit(a);
        }
        num_pending_actions++;
    }

    void submit(action* a, ThreadPool::Thread* thread)
    {
        threads->Submit(a, thread);
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
    void on_cmdlist_done(const Connection::CmdListCompletionData&);
    void on_header_update_progress(const HeaderTask::Progress&, std::size_t account);
    void on_listing_update_progress(const Listing::Progress&, std::size_t account);
    void on_write_done(const ContentTask::WriteComplete&);
};


class Engine::ConnState
{
private:
    ConnState(Engine::State& state, std::size_t cid)
    {
        logger_        = state.factory->AllocateConnectionLogger();
        thread_        = state.threads->AllocatePrivateThread();
        ui_.error      = errors::None;
        ui_.state      = states::Disconnected;
        ui_.id         = cid;
        ui_.task       = 0;
        ui_.account    = 0;
        ui_.down       = 0;
        ui_.bps        = 0;
        ui_.logfile    = logger_->GetName();
        ticks_to_ping_ = 30;
        ticks_to_conn_ = 5;
        LOG_D("Connection ", ui_.id);
    }

    using states = ui::Connection::States;
    using errors = ui::Connection::Errors;

public:
    ConnState(Engine::State& state, std::size_t aid, std::size_t cid) : ConnState(state, cid)
    {
        const auto& acc = state.find_account(aid);

        Connection::HostDetails spec;
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
        ui_.account   = aid;
        ui_.state     = ui::Connection::States::Resolving;

        conn_ = state.factory->AllocateConnection(acc);
        conn_->SetCallback(std::bind(&Engine::State::on_cmdlist_done, &state,
            std::placeholders::_1));

        do_action(state, conn_->Connect(spec));

        LOG_I("Connection ", ui_.id, " ", ui_.host, ":", ui_.port);
    }

    ConnState(Engine::State& state, std::size_t cid, const ConnState& dna) : ConnState(state, cid)
    {
        ui_.account   = dna.ui_.account;
        ui_.host      = dna.ui_.host;
        ui_.port      = dna.ui_.port;
        ui_.secure    = dna.ui_.secure;
        ui_.account   = dna.ui_.account;
        ui_.state     = ui::Connection::States::Resolving;

        const auto& acc = state.find_account(dna.ui_.account);

        Connection::HostDetails spec;
        spec.pthrottle = &state.ratecontrol;
        spec.password  = acc.password;
        spec.username  = acc.username;
        spec.enable_compression = acc.enable_compression;
        spec.enable_pipelining  = acc.enable_pipelining;
        spec.hostname = dna.ui_.host;
        spec.hostport = dna.ui_.port;
        spec.use_ssl  = dna.ui_.secure;

        conn_ = state.factory->AllocateConnection(acc);
        conn_->SetCallback(std::bind(&Engine::State::on_cmdlist_done, &state,
            std::placeholders::_1));

        do_action(state, conn_->Connect(spec));

        LOG_I("Connection ", ui_.id, " ", ui_.host, ":", ui_.port);
    }

   ~ConnState()
    {
        LOG_I("Connection ", ui_.id, " deleted");
    }

    void Tick(Engine::State& state, const std::chrono::milliseconds& elapsed)
    {
        if (ui_.state == states::Connected)
        {
            if (--ticks_to_ping_ == 0)
                do_action(state, conn_->Ping());
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
            Connection::HostDetails host;
            host.pthrottle = &state.ratecontrol;
            host.password = acc.password;
            host.username = acc.username;
            host.use_ssl  = ui_.secure;
            host.hostname = ui_.host;
            host.hostport = ui_.port;
            host.enable_compression = acc.enable_compression;
            host.enable_pipelining  = acc.enable_pipelining;
            do_action(state, conn_->Connect(host));
        }

        if (ui_.state == states::Active)
        {
            // if the bytes value hasn't increased since the last
            // read then the bps value cannot be valid but has become
            // stale since the connection has not ready any data. (i.e. it's stalled)
            const auto bytes_before  = ui_.down;
            const auto bytes_current = conn_->GetNumBytesTransferred();
            const auto bytes_speed   = conn_->GetCurrentSpeedBps();
            if (bytes_current != bytes_before)
            {
                ui_.bps  = bytes_speed;
                ui_.down = bytes_current;
            }
            else
            {
                ui_.bps = 0;
            }
        }
        else
        {
            ui_.bps = 0;
        }
    }

    void Disconnect(Engine::State& state)
    {
        if (ui_.state == states::Disconnected)
            return;

        conn_->Cancel();
        if (ui_.state == states::Connected || ui_.state == states::Active)
        {
            do_action(state, conn_->Disconnect());
        }

        state.threads->DetachPrivateThread(thread_);
    }

    void Execute(Engine::State& state, std::shared_ptr<CmdList> cmds)
    {
        ASSERT(ui_.state == states::Connected);

        ui_.task  = cmds->GetTaskId();
        ui_.desc  = cmds->GetDesc();
        ui_.state = states::Active;
        LOG_I("Connection ", ui_.id, " executing cmdlist ", cmds->GetCmdListId());

        do_action(state, conn_->Execute(cmds));
    }

    void GetStateUpdate(ui::Connection& ui)
    {
        ui = ui_;
    }

    void CompleteAction(Engine::State& engine_state, std::unique_ptr<action> act)
    {
        LOG_D("Connection ", ui_.id, " action ", act->get_id(), "(", act->describe(), ") complete");

        assert(act->get_owner() == ui_.id);

        ticks_to_ping_ = 30;
        ticks_to_conn_ = 5;

        try
        {
            if (act->has_exception())
                act->rethrow();
        }
        catch (const std::system_error& e)
        {
            LOG_E("Connection ", ui_.id, " (", e.code().value(), ") ", e.code().message());
            ui_.state = states::Error;
            ui_.bps   = 0;
            ui_.task  = 0;
            ui_.desc  = "";
            if (engine_state.on_error_callback)
            {
                ui::SystemError error;
                error.resource = ui_.host;
                error.code     = e.code();
                error.what     = e.what();
                engine_state.on_error_callback(error);
            }
            return;
        }
        catch (const std::exception& e)
        {
            LOG_E("Connection ", ui_.id, " ", e.what());
            ui_.state = states::Error;
            ui_.bps   = 0;
            ui_.task  = 0;
            ui_.desc  = "";
            if (engine_state.on_error_callback)
            {
                ui::SystemError error;
                error.resource = ui_.host;
                error.what     = e.what();
                engine_state.on_error_callback(error);
            }
            return;
        }

        auto next  = conn_->Complete(std::move(act));
        auto state = conn_->GetState();
        if (state == Connection::State::Error)
        {
            const auto err = conn_->GetError();
            ui_.state = states::Error;
            ui_.bps   = 0;
            ui_.task  = 0;
            ui_.desc  = "";
            std::string what;
            switch (err)
            {
                case Connection::Error::None:
                    ASSERT("???");
                    break;
                case Connection::Error::Resolve:
                    ui_.error = ui::Connection::Errors::Resolve;
                    what = "Failed to resolve host.";
                    break;
                case Connection::Error::Refused:
                    ui_.error = ui::Connection::Errors::Refused;
                    what = "Connection was refused.";
                    break;
                case Connection::Error::AuthenticationRejected:
                    ui_.error = ui::Connection::Errors::AuthenticationRejected;
                    what = "Session authentication was rejected.";
                    break;
                case Connection::Error::PermissionDenied:
                    ui_.error = ui::Connection::Errors::NoPermission;
                    what = "Session permission denied. Out of quota?";
                    break;
                case Connection::Error::Network:
                    ui_.error = ui::Connection::Errors::Network;
                    what = "Connection was closed unexpectedly.";
                    break;
                case Connection::Error::Timeout:
                    ui_.error = ui::Connection::Errors::Timeout;
                    what = "Connection timed out.";
                    break;
                case Connection::Error::PipelineReset:
                    ui_.error = ui::Connection::Errors::Other;
                    what = "Pipelined commands forced connection reset.";
                    break;
                case Connection::Error::Reset:
                    ui_.error = ui::Connection::Errors::Other;
                    what = "Connection was reset.";
                    break;
                case Connection::Error::Protocol:
                    ui_.error = ui::Connection::Errors::Other;
                    what = "NNTP protocol error.";
                    break;
                case Connection::Error::Other:
                    ui_.error = ui::Connection::Errors::Other;
                    what = "Unknown connection error.";
                    break;
            }
            if (engine_state.on_error_callback)
            {
                ui::SystemError error;
                error.resource = ui_.host;
                error.what     = what;
                engine_state.on_error_callback(error);
            }
        }
        else
        {
            if (state != Connection::State::Active)
            {
                ui_.bps  = 0;
                ui_.task = 0;
                ui_.desc = "";
            }
            switch (state)
            {
                case Connection::State::Disconnected:
                    ui_.state = states::Disconnected;
                    break;
                case Connection::State::Resolving:
                    ui_.state = states::Resolving;
                    break;
                case Connection::State::Connecting:
                    ui_.state = states::Connecting;
                    break;
                case Connection::State::Initializing:
                    ui_.state = states::Initializing;
                    break;
                case Connection::State::Connected:
                    ui_.state = states::Connected;
                    break;
                case Connection::State::Active:
                    ui_.state = states::Active;
                    break;
                case Connection::State::Error:
                    ASSERT("???"); // handled above.
                    break;
            }
            LOG_D("Connection ", ui_.id,  " => ", str(ui_.state));

            do_action(engine_state, std::move(next));
        }

        logger_->Flush();
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

    std::string username() const
    { return conn_->GetUsername(); }

    std::string password() const
    { return conn_->GetPassword(); }

private:
    void do_action(Engine::State& state, std::unique_ptr<action> a)
    {
        if (!a) return;

        LOG_D("Connection ", ui_.id, " current task ", ui_.task, " (", ui_.desc, ")");
        LOG_D("Connection ", ui_.id, " new action ", a->get_id(), "(", a->describe(), ")");
        a->set_owner(ui_.id);
        a->set_log(logger_);
        state.submit(a.release(), thread_);
    }

private:
    ui::Connection ui_;
    std::unique_ptr<Connection> conn_;
    std::shared_ptr<Logger> logger_;
    ThreadPool::Thread* thread_ = nullptr;
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
        Connection::HostDetails spec;
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
        conn_   = state.factory->AllocateConnection(account);
        logger_ = std::make_shared<BufferLogger>();
        id_     = id;
        success_ = false;
        finished_ = false;

        do_action(state, conn_->Connect(spec));
    }
    void on_action(Engine::State& state, std::unique_ptr<action> act)
    {
        assert(act->get_owner() == id_);

        if (state.on_test_log_callback)
        {
            for (size_t i=0; i<logger_->GetNumLines(); ++i)
            {
                const auto& msg = logger_->GetLine(i);
                state.on_test_log_callback(msg);
            }
        }
        logger_->Clear();

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

        auto next = conn_->Complete(std::move(act));
        if (next)
        {
            do_action(state, std::move(next));
        }
        else
        {
            const auto status = conn_->GetState();
            success_  = status != Connection::State::Error;
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
    std::unique_ptr<Connection> conn_;
    std::shared_ptr<BufferLogger> logger_;
    std::size_t id_;
private:
    bool success_  = false;
    bool finished_ = false;
};

class Engine::TaskState
{
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

    TaskState(std::size_t id, std::unique_ptr<Task> task, const ui::Download& desc)
    {
        task_          = std::move(task);
        ui_.task_id    = id;
        ui_.account    = desc.account;
        ui_.state      = states::Queued;
        ui_.size       = desc.size;
        ui_.desc       = desc.desc;
        ui_.path       = desc.path;
        ui_.completion = 0.0f;
        LOG_I("Task ", ui_.task_id, " (", ui_.desc, ") created");
    }

    TaskState(std::unique_ptr<Task> task, const data::TaskState& state)
    {
        task_              = std::move(task);
        ui_.completion     = task_->GetProgress();
        ui_.task_id        = state.task_id();
        ui_.batch_id       = state.batch_id();
        ui_.account        = state.account_id();
        ui_.desc           = state.desc();
        ui_.size           = state.size();
        ui_.path           = state.path();
        ui_.error.set_from_value(state.errors());
        ui_.state          = static_cast<states>(state.state());
        ui_.runtime        = state.runtime();
        if (ui_.state == states::Active ||
            ui_.state == states::Waiting ||
            ui_.state == states::Crunching)
            ui_.state = states::Waiting;

        LOG_I("Task ", ui_.task_id, "( ", ui_.desc, ") restored");
    }

   ~TaskState()
    {
        ASSERT(locking_ == LockState::Unlocked);
        LOG_I("Task ", ui_.task_id, " deleted");
    }

    void Lock(Engine::State& state)
    {
        ASSERT(locking_ == LockState::Unlocked);
        task_->Lock();
        locking_ = LockState::Locked;
    }

    void Unlock(Engine::State& state)
    {
        ASSERT(locking_ == LockState::Locked);
        task_->Unlock();
        locking_ = LockState::Unlocked;
    }

    void Serialize(data::TaskList& list)
    {
        ASSERT(num_active_actions_ == 0);
        ASSERT(locking_ == LockState::Unlocked);

        if (!task_->CanSerialize())
            return;

        std::string data;
        task_->Pack(&data);

        auto* ptr = list.add_tasks();
        ptr->set_task_id(ui_.task_id);
        ptr->set_account_id(ui_.account);
        ptr->set_batch_id(ui_.batch_id);
        ptr->set_desc(ui_.desc);
        ptr->set_size(ui_.size);
        ptr->set_path(ui_.path);
        ptr->set_errors(ui_.error.value());
        ptr->set_state(static_cast<::google::protobuf::uint32>(ui_.state));
        ptr->set_runtime(ui_.runtime);
        ptr->set_type(1); // TODO: put a dummy here for now since we're only saving downloads
        ptr->set_data(data);
    }

    void Kill(Engine::State& state)
    {
        LOG_D("Task ", ui_.task_id, " kill");

        if (ui_.state != states::Complete)
        {
            for (auto& cmd : state.cmds)
            {
                if (cmd->GetTaskId() == ui_.task_id)
                    cmd->Cancel();
            }
            task_->Cancel();

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

    void Configure(const Task::Settings& settings)
    {
        task_->Configure(settings);
    }

    void Tick(Engine::State& state, const std::chrono::milliseconds& elapsed)
    {
        task_->Tick();

        // todo: actually measure the time
        if (ui_.state == states::Active || ui_.state == states::Crunching)
            ui_.runtime++;

        ui_.has_completion = task_->HasProgress();
        ui_.completion = task_->GetProgress();
        ui_.completion = clamp(ui_.completion, 100.0);

        if ((ui_.state == states::Active ||
             ui_.state == states::Waiting ||
             ui_.state == states::Crunching) &&
             ui_.runtime >= 10 &&
             ui_.completion > 0.0)
        {
            const auto complete  = ui_.completion;
            const auto remaining = 100.0 - complete;
            const auto timespent = ui_.runtime;
            const auto time_per_percent = timespent / complete;
            const auto timeremains = remaining * time_per_percent;
            ui_.etatime = (std::uint32_t)timeremains;
        }
        else
        {
            ui_.etatime = 0;
        }
    }

    bool CanRun() const
    {
        if (ui_.state == states::Waiting ||
            ui_.state == states::Queued ||
            ui_.state == states::Active)
        {
            return task_->HasCommands();
        }
        return false;
    }

    std::shared_ptr<CmdList> CreateCommands()
    {
        return task_->CreateCommands();
    }

    transition Run(Engine::State& state)
    {
        ASSERT(ui_.state == states::Waiting ||
               ui_.state == states::Queued  ||
               ui_.state == states::Active);

        return GotoState(state, states::Active);
    }

    transition UpdateActiveState(Engine::State& state)
    {
        ASSERT(ui_.state == states::Active ||
               ui_.state == states::Paused ||
               ui_.state == states::Waiting ||
               ui_.state == states::Crunching);

        if (ui_.state != states::Active)
            return no_transition;

        for (const auto& cmd : state.cmds)
        {
            if (cmd->IsActive() &&
                cmd->GetTaskId() == ui_.task_id)
                return no_transition;
        }
        return GotoState(state, states::Waiting);
    }

    transition Complete(Engine::State& state, std::shared_ptr<CmdList> cmds)
    {
        if (ui_.state == states::Error)
            return no_transition;

        for (size_t i=0; i<cmds->NumBuffers(); ++i)
        {
            const auto& buff  = cmds->GetBuffer(i);
            const auto status = buff.GetContentStatus();
            if (status == Buffer::Status::Dmca)
                ui_.error.set(ui::TaskDesc::Errors::Dmca);
            else if (status == Buffer::Status::Unavailable)
                ui_.error.set(ui::TaskDesc::Errors::Incomplete);
            else if (status == Buffer::Status::Error)
                ui_.error.set(ui::TaskDesc::Errors::Other);

            if (status == Buffer::Status::Success)
                did_receive_content_ = true;
        }
        std::vector<std::unique_ptr<action>> actions;
        task_->Complete(*cmds, actions);
        for (auto& a : actions)
        {
            DoAction(state, std::move(a));
        }

        return ChooseState(state);
    }

    transition Pause(Engine::State& state)
    {
        if (ui_.state == states::Paused ||
            ui_.state == states::Error  ||
            ui_.state == states::Complete)
            return no_transition;

        for (auto& cmd : state.cmds)
        {
            if (cmd->GetTaskId() == ui_.task_id)
                cmd->Cancel();
        }

        return GotoState(state, states::Paused);
    }

    transition Resume(Engine::State& state)
    {
        if (ui_.state != states::Paused)
            return no_transition;

        // it's possible that user paused the task but all
        // command were already processed by the background
        // threads but are queued waiting for state transition.
        // then the task completes while it's in the paused state.
        // hence we expect the completion on resume and possibly
        // transfer to complete stated instead of waiting/queued.
        const auto transition = ChooseState(state);
        if (transition)
            return transition;

        // if we were started before we go to waiting state.
        if (ui_.runtime)
            return GotoState(state, states::Waiting);

        // go back into queued state.
        return GotoState(state, states::Queued);
    }

    void GetStateUpdate(ui::TaskDesc& ui)
    {
        ui = ui_;
    }

    transition Complete(Engine::State& state, std::unique_ptr<action> act)
    {
        const auto size = act->size();

        assert(act->get_owner() == ui_.task_id);
        assert(num_active_actions_ > 0);
        assert(size <= num_bytes_queued_);

        num_active_actions_--;
        num_bytes_queued_ -= size;

        LOG_D("Task ", ui_.task_id, " receiving action ", act->get_id(), " (", act->describe(), ")");
        LOG_D("Task ", ui_.task_id, " num_active_actions ", num_active_actions_);
        LOG_D("Task ", ui_.task_id, " has ", num_bytes_queued_ / (1024.0*1024.0), " Mb queued");

        if (ui_.state == states::Error)
            return no_transition;

        ui::SystemError error;
        error.resource = ui_.desc;
        try
        {
            if (act->has_exception())
                act->rethrow();

            std::vector<std::unique_ptr<action>> actions;
            task_->Complete(*act, actions);
            act.reset();

            const auto err = task_->GetErrors();
            if (err.test(Task::Error::CrcMismatch))
                ui_.error.set(ui::TaskDesc::Errors::Damaged);
            if (err.test(Task::Error::SizeMismatch))
                ui_.error.set(ui::TaskDesc::Errors::Damaged);

            for (auto& a : actions)
            {
                DoAction(state, std::move(a));
            }

            return ChooseState(state);
        }
        catch (const std::system_error& e)
        {
            LOG_E("Task ", ui_.task_id, " Error: ", error.what);

            if (state.on_error_callback)
            {
                ui::SystemError error;
                error.resource = ui_.desc;
                error.code = e.code();
                error.what = e.what();
                state.on_error_callback(error);
            }
            return GotoState(state, states::Error);
        }
        catch (const std::exception& e)
        {
            LOG_E("Task ", ui_.task_id, " Error: ", error.what);

            if (state.on_error_callback)
            {
                ui::SystemError error;
                error.resource = ui_.desc;
                error.what = e.what();
                state.on_error_callback(error);
            }
            return GotoState(state, states::Error);
        }

        return no_transition;
    }

    void SetAccountId(std::size_t acc)
    { ui_.account = acc; }

    void SetBatchId(std::size_t batch)
    { ui_.batch_id = batch; }

    std::size_t GetAccountId() const
    { return ui_.account; }

    std::size_t GetTaskId() const
    { return ui_.task_id; }

    std::size_t GetBatchId() const
    { return ui_.batch_id; }

    states GetState() const
    { return ui_.state; }

    std::string GetDesc() const
    { return ui_.desc; }

    std::uint64_t GetSize() const
    { return ui_.size; }

    bitflag<ui::TaskDesc::Errors> errors() const
    { return ui_.error; }

    double GetCompletion() const
    { return task_->GetProgress(); }

    bool HasCompletion() const
    { return task_->HasProgress(); }

    bool CanSerialize() const
    { return task_->CanSerialize(); }

    bool IsDamaged() const
    { return damaged_; }

    std::size_t NumFiles() const
    { return num_files_produced_; }

private:
    transition ChooseState(Engine::State& state)
    {
        // if we have pending actions our state should not change
        // i.e. we're active (or possibly paused)
        if (num_active_actions_)
        {
            if (num_bytes_queued_ >= MB(64))
            {
                if (ui_.state == states::Active)
                    return GotoState(state, states::Crunching);
            }
            return no_transition;
        }

        // if we have currently executing cmdlists our state should not change.
        // i.e. we're active (or possibly paused)
        const auto num_active_cmdlists = std::count_if(std::begin(state.cmds), std::end(state.cmds),
            [&](const std::shared_ptr<CmdList>& cmdlist) {
                return cmdlist->GetConnId() != 0 &&
                       cmdlist->GetTaskId() == ui_.task_id;
            });
        if (num_active_cmdlists)
            return no_transition;


        // if task list has more commands and there are no currently
        // executing cmdlists or actions and we're not paused
        // we'll transition into waiting state.
        if (task_->HasCommands())
        {
            if (ui_.state == states::Active ||
                ui_.state == states::Crunching)
                return GotoState(state, states::Waiting);
        }
        else
        {
            return GotoState(state, states::Complete);
        }
        return no_transition;
    }

    void DoAction(Engine::State& state, std::unique_ptr<action> a)
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

    transition GotoState(Engine::State& state, states new_state)
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
                for (auto& cmd : state.cmds)
                {
                    if (cmd->GetTaskId() == ui_.task_id)
                        cmd->Cancel();
                }
                break;

            case states::Complete:
            {
                ASSERT(task_->HasCommands() == false);
                ASSERT(num_active_actions_ == 0);

                task_->Commit();

                ui_.error.set(ui::TaskDesc::Errors::Unavailable, !did_receive_content_);
                ui_.state   = new_state;
                ui_.etatime = 0;
                if (state.on_task_callback)
                    state.on_task_callback(ui_);

                state.bytes_ready += ui_.size;

                auto result = state.factory->MakeResult(*task_, ui_);
                if (result)
                {
                    ui::Result* result_ptr = result.get();

                    if (auto* ptr = dynamic_cast<ui::FileResult*>(result_ptr))
                    {
                        state.on_file_callback(*ptr);
                        for (const auto& file : ptr->files)
                        {
                            damaged_ |= file.damaged;
                        }
                        num_files_produced_ = ptr->files.size();
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
                task_->Cancel();

                ui_.state   = new_state;
                ui_.etatime = 0;
                if (state.on_task_callback)
                    state.on_task_callback(ui_);

                for (auto& cmd : state.cmds)
                {
                    if (cmd->GetTaskId() == ui_.task_id)
                        cmd->Cancel();
                }
                break;
        }

        LOG_D("Task ", ui_.task_id, " => ", str(new_state));
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
    std::unique_ptr<newsflash::Task> task_;
    std::size_t num_active_actions_  = 0;
    std::size_t num_bytes_queued_    = 0;
    std::size_t num_files_produced_  = 0;
    bool did_receive_content_ = false;
    bool damaged_ = false;

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
public:
    using states = ui::TaskDesc::States;

    enum class Type {
        FileBatch,
        ListBatch,
        UpdateBatch
    };

    BatchState(std::size_t id, const ui::FileBatchDownload& files)
    {
        ui_.task_id    = id;
        ui_.batch_id   = id;
        ui_.account    = files.account;
        ui_.desc       = files.desc;
        ui_.path       = files.path;
        ui_.size       = files.size;
        ui_.completion = 0.0f;
        num_tasks_     = files.files.size();
        type_ = Type::FileBatch;

        LOG_I("Batch ", ui_.batch_id, " (", ui_.desc, ") created");
        LOG_D("Batch has ", num_tasks_, " tasks");
    }

    BatchState(std::size_t id, const ui::GroupListDownload& list)
    {
        ui_.task_id    = id;
        ui_.batch_id   = id;
        ui_.account    = list.account;
        ui_.desc       = list.desc;
        ui_.path       = list.path;
        ui_.size       = list.size;
        ui_.completion = 0.0f;
        num_tasks_   = 1;
        type_ = Type::ListBatch;

        LOG_I("Batch ", ui_.batch_id, " (", ui_.desc, ") created");
        LOG_D("Batch has 1 task");
    }

    BatchState(std::size_t id, const ui::HeaderDownload& download)
    {
        ui_.task_id  = id;
        ui_.batch_id = id;
        ui_.account  = download.account;
        ui_.desc     = download.desc;
        ui_.path     = download.path;
        ui_.size     = download.size;
        ui_.completion = 0.0f;
        num_tasks_   = 1;
        type_ = Type::UpdateBatch;

        LOG_I("Batch ", ui_.batch_id, " (", ui_.desc, ") created");
        LOG_D("Batch has 1 tasks");
    }

    BatchState(const data::Batch& data)
    {
        ui_.batch_id   = data.batch_id();
        ui_.task_id    = data.batch_id();
        ui_.account    = data.account_id();
        ui_.desc       = data.desc();
        ui_.path       = data.path();
        ui_.size       = data.byte_size();
        ui_.completion = data.completion();
        ui_.runtime    = data.runtime();
        num_tasks_     = data.num_tasks();
        type_          = static_cast<Type>(data.type());
        damaged_       = data.damaged();
    }

   ~BatchState()
    {
        LOG_I("Batch ", ui_.batch_id, " deleted");
    }

    void Serialize(data::TaskList& list)
    {
        auto* data = list.add_batch();
        data->set_batch_id(ui_.task_id);
        data->set_account_id(ui_.account);
        data->set_desc(ui_.desc);
        data->set_byte_size(ui_.size);
        data->set_path(ui_.path);
        data->set_runtime(ui_.runtime);
        data->set_completion(ui_.completion);
        data->set_num_tasks(num_tasks_);
        data->set_num_files(num_files_);
        data->set_damaged(damaged_);
        data->set_type(static_cast<::google::protobuf::uint32>(type_));

    }

    void lock(Engine::State& state)
    {
        LOG_D("Batch lock ", ui_.batch_id);

        for (auto& task : state.tasks)
        {
            if (task->GetBatchId() != ui_.batch_id)
                continue;

            task->Lock(state);
        }
    }

    void unlock(Engine::State& state)
    {
        LOG_D("Batch unlock ", ui_.batch_id);

        for (auto& task : state.tasks)
        {
            if (task->GetBatchId() != ui_.batch_id)
                continue;

            task->Unlock(state);
        }
    }

    void pause(Engine::State& state)
    {
        LOG_D("Batch pause ", ui_.batch_id);

        for (auto& task : state.tasks)
        {
            if (task->GetBatchId() != ui_.batch_id)
                continue;

            task->Pause(state);
        }
        UpdateState(state);
    }

    void resume(Engine::State& state)
    {
        LOG_D("Batch resume ", ui_.batch_id);

        for (auto& task : state.tasks)
        {
            if (task->GetBatchId() != ui_.batch_id)
                continue;

            task->Resume(state);
        }
        UpdateState(state);
    }

    void GetStateUpdate(ui::TaskDesc& ui)
    {
        ui = ui_;
        ui.completion += partial_task_completion_;
        ui.completion = clamp(ui.completion, 100.0);
    }

    void kill(Engine::State& state)
    {
        LOG_D("Batch kill ", ui_.batch_id);

        // partition tasks so that the tasks that belong to this batch
        // are at the end of the the task list.
        auto it = std::stable_partition(std::begin(state.tasks), std::end(state.tasks),
            [&](const std::unique_ptr<TaskState>& t) {
                return t->GetBatchId() != ui_.batch_id;
            });

        for (auto i=it; i != std::end(state.tasks); ++i)
            (*i)->Kill(state);

        state.tasks.erase(it, std::end(state.tasks));
    }

    bool HasTasks(Engine::State& state) const
    {
        for (const auto& task : state.tasks)
        {
            if (task->GetBatchId() == ui_.batch_id)
                return true;
        }
        return false;
    }

    void UpdateState(Engine::State& state, const TaskState& task, const TaskState::transition& s)
    {
        ui_.error |= task.errors();

        if (s.current == states::Complete || s.current == states::Error)
        {
            const auto task_size = task.GetSize();
            const auto this_size = ui_.size;
            const auto compute_based_on_actual_size = task_size != 0 && this_size != 0;

            const double task_slice_size = compute_based_on_actual_size ?
                (double)task_size / (double)this_size : 1.0 / (double)num_tasks_;

            const double task_completion = task.HasCompletion() ? task.GetCompletion() : 100.0f;
            const double actually_done   = task_slice_size * task_completion; // * 100.0f;
            ui_.completion += actually_done;
            ui_.completion = clamp(ui_.completion, 100.0);

            num_files_ += task.NumFiles();
            damaged_   |= task.IsDamaged();
        }
        UpdateState(state);
    }

    void tick(Engine::State& state, const std::chrono::milliseconds& elapsed)
    {
        if (ui_.state == states::Active || ui_.state == states::Crunching)
            ui_.runtime++;

        ui_.has_completion = true;

        partial_task_completion_ = 0;

        // compute partial running task completion
        for (const auto& task : state.tasks)
        {
            if (task->GetBatchId() != ui_.batch_id)
                continue;

            ui_.has_completion = ui_.has_completion & task->HasCompletion();

            const auto state = task->GetState();
            if (state == states::Error || state == states::Complete)
                continue;

            const auto task_size = task->GetSize();
            const auto this_size = ui_.size;
            const auto compute_based_on_actual_size = task_size != 0 && this_size != 0;

            const double task_slice_size = compute_based_on_actual_size ?
                (double)task_size / (double)this_size : 1.0 / (double)num_tasks_;

            const double task_completion = task->GetCompletion();
            const double actually_done   = task_slice_size * task_completion;// * 100.0f;
            partial_task_completion_ += actually_done;
        }

        if ((ui_.state == states::Active ||
             ui_.state == states::Waiting ||
             ui_.state == states::Crunching) &&
             ui_.runtime >= 10 &&
             ui_.completion + partial_task_completion_ > 0.0)
        {
            // compute eta.
            const auto complete = ui_.completion + partial_task_completion_;
            const auto remaining = 100.0 - complete;
            const auto timespent = ui_.runtime;
            const auto time_per_percent = timespent / complete;
            const auto time_remains = remaining * time_per_percent;
            ui_.etatime = (std::uint32_t)time_remains;
        }
        else
        {
            ui_.etatime = 0;
        }
    }

    std::size_t id() const
    { return ui_.batch_id; }

    void UpdateState(Engine::State& state, bool run_callbacks = true)
    {
        size_t num_queued_tasks  = 0;
        size_t num_waiting_tasks = 0;
        size_t num_active_tasks  = 0;
        size_t num_crunching_tasks = 0;
        size_t num_paused_tasks = 0;
        size_t num_complete_tasks = 0;
        size_t num_error_tasks = 0;
        size_t num_tasks = 0;

        for (const auto& task : state.tasks)
        {
            if (task->GetBatchId() != ui_.batch_id)
                continue;
            switch (task->GetState())
            {
                case states::Queued:
                    num_queued_tasks++;
                    break;
                case states::Waiting:
                    num_waiting_tasks++;
                    break;
                case states::Active:
                    num_active_tasks++;
                    break;
                case states::Crunching:
                    num_crunching_tasks++;
                    break;
                case states::Paused:
                    num_paused_tasks++;
                    break;
                case states::Complete:
                    num_complete_tasks++;
                    break;
                case states::Error:
                    num_error_tasks++;
                    break;
            }
            num_tasks++;
        }

        LOG_D("Batch has ", num_tasks, " tasks");
        LOG_D("Batch tasks"
              " Queued: ", num_queued_tasks,
              " Waiting: ", num_waiting_tasks,
              " Active: ", num_active_tasks,
              " Crunching: ", num_crunching_tasks,
              " Paused: ", num_paused_tasks,
              " Complete: ", num_complete_tasks,
              " Errored: ", num_error_tasks);

        if (num_active_tasks)
            GotoState(state, states::Active, run_callbacks);
        else if (num_tasks == num_queued_tasks)
            GotoState(state, states::Queued, run_callbacks);
        else if (num_tasks == num_active_tasks)
            GotoState(state, states::Active, run_callbacks);
        else if (num_tasks == num_crunching_tasks)
            GotoState(state, states::Crunching, run_callbacks);
        else if (num_tasks == num_paused_tasks)
            GotoState(state, states::Paused, run_callbacks);
        else if (num_tasks == num_complete_tasks)
            GotoState(state, states::Complete, run_callbacks);
        else if (num_tasks == num_error_tasks)
            GotoState(state, states::Error, run_callbacks);
        else if (num_tasks == num_error_tasks + num_complete_tasks)
            GotoState(state, states::Error, run_callbacks);
        else if (num_waiting_tasks)
            GotoState(state, states::Waiting, run_callbacks);
        else if (num_queued_tasks)
            GotoState(state, states::Queued, run_callbacks);
        else if (num_paused_tasks)
            GotoState(state, states::Paused, run_callbacks);
        else if (num_crunching_tasks)
            GotoState(state, states::Crunching, run_callbacks);
    }
private:
    void GotoState(Engine::State& state, states new_state, bool run_callbacks = true)
    {
        ui_.state = new_state;
        switch (new_state)
        {
            case states::Error:
            case states::Complete:
                if (state.on_batch_callback && type_ == Type::FileBatch && run_callbacks)
                {
                    ui::FileBatchResult result;
                    result.account   = ui_.account;
                    result.path      = ui_.path;
                    result.desc      = ui_.desc;
                    result.filecount = num_files_;
                    result.damaged   = damaged_;
                    state.on_batch_callback(result);
                }
                break;

            default:
            break;
        }
        LOG_D("Batch ", ui_.batch_id, " => ", str(new_state));
    }
private:
    ui::TaskDesc ui_;
private:
    std::size_t num_tasks_ = 0;
    std::size_t num_files_ = 0;
    bool damaged_ = false;
    float partial_task_completion_ = 0.0f;
    Type type_ = Type::FileBatch;
};

void Engine::State::on_cmdlist_done(const Connection::CmdListCompletionData& completion)
{
    auto cmds = completion.cmds;
    const auto bytes = completion.content_bytes;
    const auto tid   = cmds->GetTaskId();
    const auto id    = cmds->GetCmdListId();

    bytes_downloaded += bytes;

    if (cmds->GetType() == CmdList::Type::Article)
    {
        if (on_quota_callback && bytes)
            on_quota_callback(bytes, cmds->GetAccountId());
    }
    LOG_D("Cmdlist ", id, " executed");
    LOG_D("Cmdlist ", id, " belongs to task ", cmds->GetTaskId());
    LOG_D("Cmdlist ", id, " goodbit: ", cmds->IsGood());
    LOG_D("Cmdlist ", id, " cancelbit: ", cmds->IsCancelled());
    LOG_D("Cmdlist ", id, " completed: ", completion.execution_did_complete);
    LOG_D("Cmdlist ", id, " has failed content: ", cmds->HasFailedContent());

    #ifdef NEWSFLASH_DEBUG
    if (std::getenv("NEWSFLASH_DUMP_DATA"))
    {
        const auto& buffers  = cmds->GetBuffers();
        const auto& commands = cmds->GetCommands();
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
            if (buff.GetContentStatus() == Buffer::Status::Success)
                out.write(buff.Content(), buff.GetContentLength()-3);
            out.flush();
        }
    }
    #endif

    // remove the cmdlist from the list of command lists to execute.
    auto& current_cmdlists = this->cmds;
    current_cmdlists.erase(std::remove(std::begin(current_cmdlists), std::end(current_cmdlists), cmds),
        std::end(current_cmdlists));

    // find the task that the cmdlist belongs to, note that it could have been deleted.
    auto it  = std::find_if(std::begin(tasks), std::end(tasks),
        [&](const std::unique_ptr<TaskState>& t) {
            return t->GetTaskId() == tid;
        });
    if (it == std::end(tasks))
        return;

    auto& task = *it;

    // there are several ways a command list execution can go.
    //
    // - cmdlist could not run because the connection failed
    //   completion data will indicate this by execution_did_complete == false
    //
    // - cmdlist was canceled because the task was for example paused.
    //   cmdlist will contain buffers that have not yet run, i.e. their statuses are None
    //
    // - cmdlist failed to configure, i.e. the server didn't have the specified newsgroup
    //   all buffers will not have run so their statuses are None
    //
    // - cmdlist ran to completion
    //   all buffers have non None statuses

    if (completion.execution_did_complete)
    {
        const auto cancelled  = cmds->IsCancelled();
        const auto configured = cmds->IsGood();

        // if the cmdlist failed to configure, i.e. the newsgroups were not available
        // we're going to interpret this as being a condition where none of the buffers
        // were available.
        if (!cancelled && !configured)
        {
            for (size_t i=0; i<cmds->NumBuffers(); ++i)
            {
                auto& buffer = cmds->GetBuffer(i);
                buffer.SetStatus(Buffer::Status::Unavailable);
            }
        }

        const auto failed    = cmds->HasFailedContent();
        const auto fillable  = cmds->IsFillable();
        const auto filling_enabled = (this->fill_account != 0);
        if (!cancelled && failed && fillable && filling_enabled)
        {
            const auto account = cmds->GetAccountId();
            if (account != fill_account)
            {
                LOG_D("Cmdlist ", id, " set for refill");
                cmds->SetAccountId(this->fill_account);
                cmds->SetConnId(0);
                current_cmdlists.push_back(cmds);
                const auto transition = task->UpdateActiveState(*this);
                if (transition)
                {
                    auto* batch = find_batch(task->GetBatchId());
                    batch->UpdateState(*this, *task, transition);
                }
                return;
            }
        }

        const auto transition = task->Complete(*this, cmds);
        if (transition)
        {
            auto* batch = find_batch(task->GetBatchId());
            batch->UpdateState(*this, *task, transition);
        }
    }
    else
    {
        // the command list didn't run properly, enqueue it again
        // to be run on some other connection.
        cmds->SetConnId(0);
        current_cmdlists.push_back(cmds);
        const auto transition = task->UpdateActiveState(*this);
        if (transition)
        {
            auto* batch = find_batch(task->GetBatchId());
            batch->UpdateState(*this, *task, transition);
        }
    }
}

void Engine::State::on_listing_update_progress(const Listing::Progress& progress, std::size_t account)
{
    if (!on_listing_update_callback)
        return;

    ui::GroupListUpdate update;
    update.account = account;

    for (const auto& group : progress.groups)
    {
        ui::GroupListUpdate::NewsGroup data;
        data.first = group.first;
        data.last  = group.last;
        data.name  = group.name;
        data.size  = group.size;
        update.groups.push_back(std::move(data));
    }
    on_listing_update_callback(update);
}

void Engine::State::on_header_update_progress(const HeaderTask::Progress& progress, std::size_t account)
{
    if (!on_header_update_callback)
        return;

    ui::HeaderUpdate update;
    update.account    = account;
    update.group_name = progress.group;
    update.num_local_articles  = progress.num_local_articles;
    update.num_remote_articles = progress.num_remote_articles;
    update.local_last = progress.local_last;
    update.local_first = progress.local_first;
    update.remote_last = progress.remote_last;
    update.remote_first = progress.remote_first;
    update.catalogs = progress.catalogs;

    for (auto& snapshot : progress.snapshots)
    {
        update.snapshots.push_back(snapshot.get());
    }
    on_header_update_callback(update);
}

void Engine::State::on_write_done(const ContentTask::WriteComplete& write)
{
    bytes_written += write.size;
}

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

    // spawn connections if needed.
    for (const auto& account : accounts)
    {
        const bool has_cmds = std::find_if(std::begin(cmds), std::end(cmds),
            [&](const std::shared_ptr<CmdList>& cmdlist) {
                return cmdlist->GetAccountId() == account.id;
            }) != std::end(cmds);
        const bool has_tasks = std::find_if(std::begin(tasks), std::end(tasks),
            [&](const std::unique_ptr<TaskState>& task) {
                return task->GetAccountId() == account.id &&
                       task->CanRun();
            }) != std::end(tasks);

        if (has_cmds || has_tasks)
        {
            std::size_t num_conns = std::count_if(std::begin(conns), std::end(conns),
                [&](const std::unique_ptr<ConnState>& conn) {
                    return conn->account() == account.id;
                });
            for (; num_conns < account.connections; ++num_conns)
            {
                conns.emplace_back(new ConnState(*this, account.id, oid++));
            }
        }
    }

    // schedule cmdlists (if any) to available connections
    for (auto& conn : conns)
    {
        if (!conn->is_ready())
            continue;

        const auto account = conn->account();

        std::shared_ptr<CmdList> cmdlist;
        TaskState* task = nullptr;

        auto it = std::find_if(std::begin(cmds), std::end(cmds),
            [&](const std::shared_ptr<CmdList>& cmdlist) {
                return cmdlist->GetConnId() == 0 &&
                       cmdlist->GetAccountId() == conn->account() &&
                       cmdlist->IsCancelled() == false;
            });
        if (it != std::end(cmds))
        {
            cmdlist = *it;
            auto it = std::find_if(std::begin(tasks), std::end(tasks),
                [&](const std::unique_ptr<TaskState>& task) {
                    return task->GetTaskId() == cmdlist->GetTaskId();
                });
            ASSERT(it != std::end(tasks));
            task = (*it).get();
        }
        else
        {
            // look for the next runnable task.
            auto it = std::find_if(std::begin(tasks), std::end(tasks),
                [&](const std::unique_ptr<TaskState>& task) {
                    return task->GetAccountId() == account &&
                           task->CanRun();
                });
            if (it != std::end(tasks))
            {
                task = (*it).get();
                cmdlist = task->CreateCommands();
                cmds.push_back(cmdlist);
                const auto transition = task->Run(*this);
                if (transition)
                {
                    auto* batch = find_batch(task->GetBatchId());
                    batch->UpdateState(*this, *task, transition);
                }
            }
        }

        if (!cmdlist)
            continue;

        cmdlist->SetAccountId(account);
        cmdlist->SetTaskId(task->GetTaskId());
        cmdlist->SetDesc(task->GetDesc());
        cmdlist->SetConnId(conn->id());
        conn->Execute(*this, cmdlist);
    }
}

Engine::Engine(std::unique_ptr<Factory> factory,
    bool enable_single_thread_debug) : state_(new State(enable_single_thread_debug))
{
    state_->factory = std::move(factory);
    state_->logger  = state_->factory->AllocateEngineLogger();

    // todo: we should really set the logger on every entry point
    // (i.e. set the log to our log and then restore on return)
    // theoretically the client could use different threads to call
    // into the engine.
    if (state_->logger->IsOpen())
    {
        SetThreadLog(state_->logger.get());
    }
    else
    {
        SetThreadLog(nullptr);
    }
}

Engine::Engine(const std::string& logpath) : state_(new State(false))
{
    state_->factory = std::make_unique<DefaultFactory>(logpath);
    state_->logger  = state_->factory->AllocateEngineLogger();

    // todo: we should really set the logger on every entry point
    // (i.e. set the log to our log and then restore on return)
    // theoretically the client could use different threads to call
    // into the engine.
    if (state_->logger->IsOpen())
    {
        SetThreadLog(state_->logger.get());
    }
    else
    {
        SetThreadLog(nullptr);
    }
}

Engine::~Engine()
{
    assert(state_->num_pending_actions == 0);
    assert(state_->conns.empty());
    assert(state_->started == false);

    state_->threads->Shutdown();
    state_->threads.reset();
}

void Engine::TryAccount(const ui::Account& account)
{
    const auto id = state_->oid++;

    state_->current_connection_test.reset(new ConnTestState(account, id, *state_));
}

void Engine::SetAccount(const ui::Account& acc, bool spawn_connections_immediately)
{
    auto it = std::find_if(std::begin(state_->accounts), std::end(state_->accounts),
        [&](const ui::Account& a) {
            return a.id == acc.id;
        });
    if (it == std::end(state_->accounts))
    {
        state_->accounts.push_back(acc);
        if (spawn_connections_immediately)
        {
            for (size_t i=0; i<acc.connections; ++i)
            {
                const auto id = state_->oid++;
                state_->conns.emplace_back(new ConnState(*state_, acc.id, id));
            }
        }
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

    Task::Settings settings;
    settings.discard_text_content     = state_->discard_text;
    settings.overwrite_existing_files = state_->overwrite_existing;

    for (const auto& file : batch.files)
    {
        assert(file.path == batch.path);

        const auto taskid = state_->oid++;

        std::unique_ptr<Task> task(state_->factory->AllocateTask(file));
        if (auto* ptr = dynamic_cast<ContentTask*>(task.get()))
        {
            ptr->SetWriteCallback(std::bind(&Engine::State::on_write_done, state_.get(),
                std::placeholders::_1));
        }
        std::unique_ptr<TaskState> state(new TaskState(taskid, std::move(task), file));
        state->Configure(settings);
        state->SetAccountId(batch.account);
        state->SetBatchId(batchid);

        if (priority)
            state_->tasks.push_front(std::move(state));
        else state_->tasks.push_back(std::move(state));

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
    std::unique_ptr<Task> task(state_->factory->AllocateTask(list));
    if (auto* ptr = dynamic_cast<Listing*>(task.get()))
    {
        ptr->SetProgressCallback(std::bind(&Engine::State::on_listing_update_progress, state_.get(),
            std::placeholders::_1, list.account));
    }

    std::unique_ptr<TaskState> state(new TaskState(taskid, std::move(task), list));
    state->SetAccountId(list.account);
    state->SetBatchId(batchid);

    state_->tasks.push_back(std::move(state));
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
    std::unique_ptr<Task> task(state_->factory->AllocateTask(download));
    if (auto* ptr = static_cast<HeaderTask*>(task.get()))
    {
        ptr->SetProgressCallback(std::bind(&Engine::State::on_header_update_progress, state_.get(),
            std::placeholders::_1, download.account));
    }

    std::unique_ptr<TaskState> state(new TaskState(taskid, std::move(task), download));
    state->SetAccountId(download.account);
    state->SetBatchId(batchid);

    state_->tasks.push_back(std::move(state));
    state_->batches.push_back(std::move(batch));
    state_->num_pending_tasks++;
    state_->execute();
    return batchid;
}

bool Engine::Pump()
{
    state_->quit_pump_loop = false;

    SetThreadLog(state_->logger.get());

    for (;;)
    {
        if (state_->quit_pump_loop)
            break;

        std::unique_ptr<action> action = state_->get_action();
        if (!action)
            break;

        action->run_completion_callbacks();

        const auto id = action->get_owner();
        auto it = std::find_if(std::begin(state_->conns), std::end(state_->conns),
            [&](const std::unique_ptr<ConnState>& c) {
                return c->id() == id;
            });
        if (it != std::end(state_->conns))
        {
            auto& conn = *it;
            conn->CompleteAction(*state_, std::move(action));
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
                if (task->GetTaskId() != id)
                    continue;

                const auto transition = task->Complete(*state_, std::move(action));
                if (transition)
                {
                    auto* batch = state_->find_batch(task->GetBatchId());
                    batch->UpdateState(*state_, *task, transition);
                }
                break;
            }
            state_->execute();
        }
        state_->num_pending_actions--;
    }

    state_->execute();

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
                    return t->GetBatchId() == batch->id();
                });
        }

        state_->repartition_task_list = false;
    }

    for (auto& conn : state_->conns)
    {
    // todo: measure time here instead of relying on the clientr
        conn->Tick(*state_, std::chrono::seconds(1));
    }

    for (auto& task : state_->tasks)
    {
        task->Tick(*state_, std::chrono::seconds(1));
    }

    for (auto& batch : state_->batches)
    {
        batch->tick(*state_, std::chrono::seconds(1));
    }

    state_->execute();

    if (state_->logger)
        state_->logger->Flush();
}

void Engine::RunMainThread()
{
    state_->threads->RunMainThreads();
}

void Engine::Start()
{
    if (!state_->started)
    {
        LOG_I("Engine starting");
        LOG_D("Current settings:");
        LOG_D("Overwrite existing files: ", state_->overwrite_existing);
        LOG_D("Discard text content: ", state_->discard_text);
        LOG_D("Prefer secure:", state_->prefer_secure);

        state_->started = true;
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
        conn->Disconnect(*state_);
    }
    state_->conns.clear();
    state_->started = false;
    state_->logger->Flush();
}

void Engine::SaveTasks(const std::string& file)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

#if defined(WINDOWS_OS)
    // msvc specific extension..
    std::ofstream out(utf8::decode(file), std::ios::out | std::ios::binary | std::ios::trunc);
#elif defined(LINUX_OS)
    std::ofstream out(file, std::ios::out | std::ios::binary | std::ios::trunc);
#endif
    if (!out.is_open())
        throw std::system_error(errno, std::generic_category(), "failed to open: " + file);

    data::TaskList list;

    for (const auto& batch : state_->batches)
    {
        bool can_serialize = true;
        for (const auto& task : state_->tasks)
        {
            if (task->GetBatchId() != batch->id())
                continue;
            can_serialize = task->CanSerialize();
            if (!can_serialize)
                break;
        }
        if (!can_serialize)
            continue;

        batch->Serialize(list);
    }

    for (const auto& task : state_->tasks)
    {
        task->Serialize(list);
    }

    list.set_bytes_queued(state_->bytes_queued);
    list.set_bytes_ready(state_->bytes_ready);
    list.set_object_id(state_->oid);

    if (!list.SerializeToOstream(&out))
        throw std::runtime_error("engine serialize to stream failed");
}

void Engine::LoadTasks(const std::string& file)
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

    Task::Settings settings;
    settings.discard_text_content = state_->discard_text;
    settings.overwrite_existing_files = state_->overwrite_existing;

    for (int i=0; i<list.tasks_size(); ++i)
    {
        const auto& state_data = list.tasks(i);
        const auto& type = state_data.type();
        const auto& data = state_data.data();
        std::unique_ptr<Task> task(state_->factory->AllocateTask(type));
        task->Load(data);
        if (auto* ptr = dynamic_cast<ContentTask*>(task.get()))
        {
            ptr->SetWriteCallback(std::bind(&Engine::State::on_write_done, state_.get(),
                std::placeholders::_1));
        }
        std::unique_ptr<TaskState> state(new TaskState(std::move(task), state_data));
        state->Configure(settings);
        state_->tasks.push_back(std::move(state));
        state_->num_pending_tasks++;
    }

    for (int i=0; i<list.batch_size(); ++i)
    {
        const bool run_callbacks = false;

        const auto& data   = list.batch(i);
        std::unique_ptr<BatchState> batch(new BatchState(data));
        batch->UpdateState(*state_, run_callbacks);
        state_->batches.push_back(std::move(batch));
    }


    state_->bytes_queued = list.bytes_queued();
    state_->bytes_ready  = list.bytes_ready();
    state_->oid          = list.object_id();
}

void Engine::Reset()
{
    state_->tasks.clear();
    state_->batches.clear();
    state_->accounts.clear();
    state_->num_pending_tasks = 0;
    state_->bytes_queued = 0;
    state_->bytes_ready = 0;
    state_->bytes_written = 0;
    state_->oid = 1;
    state_->fill_account = 0;
    state_->prefer_secure = true;
    state_->overwrite_existing = false;
    state_->discard_text = false;
    state_->started = false;
    state_->group_items = false;
    state_->repartition_task_list = false;
    state_->quit_pump_loop = false;
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

void Engine::SetListingUpdateCallback(on_listing_update callback)
{
    state_->on_listing_update_callback = std::move(callback);
}

void Engine::SetHeaderInfoCallback(on_header_update callback)
{
    state_->on_header_update_callback = std::move(callback);
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

    Task::Settings settings;
    settings.overwrite_existing_files = on_off;
    settings.discard_text_content = state_->discard_text;

    for (auto& t: state_->tasks)
        t->Configure(settings);
}

void Engine::SetDiscardTextContent(bool on_off)
{
    state_->discard_text = on_off;

    Task::Settings settings;
    settings.discard_text_content = on_off;
    settings.overwrite_existing_files = state_->overwrite_existing;

    for (auto& t : state_->tasks)
        t->Configure(settings);
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
                    return t->GetBatchId() == batch->id();
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
    return state_->logger->GetName();
}


bool Engine::IsStarted() const
{
    return state_->started;
}

bool Engine::HasPendingActions() const
{
    return state_->num_pending_actions != 0;
}

void Engine::GetTasks(std::deque<ui::TaskDesc>* tasklist) const
{
    if (state_->group_items)
    {
        const auto& batches = state_->batches;

        tasklist->resize(batches.size());

        for (std::size_t i=0; i<batches.size(); ++i)
            batches[i]->GetStateUpdate((*tasklist)[i]);
    }
    else
    {
        const auto& tasks = state_->tasks;

        tasklist->resize(tasks.size());

        for (std::size_t i=0; i<tasks.size(); ++i)
            tasks[i]->GetStateUpdate((*tasklist)[i]);
    }
}

void Engine::GetTask(std::size_t index, ui::TaskDesc* task) const
{
    if (state_->group_items)
    {
        ASSERT(index < state_->batches.size());
        state_->batches[index]->GetStateUpdate(*task);
    }
    else
    {
        ASSERT(index < state_->tasks.size());
        state_->tasks[index]->GetStateUpdate(*task);
    }
}

void Engine::GetConns(std::deque<ui::Connection>* connlist) const
{
    const auto& conns = state_->conns;

    connlist->resize(conns.size());

    for (std::size_t i=0; i<conns.size(); ++i)
    {
        conns[i]->GetStateUpdate((*connlist)[i]);
    }
}

void Engine::GetConn(std::size_t index, ui::Connection* conn) const
{
    ASSERT(index < state_->conns.size());

    state_->conns[index]->GetStateUpdate(*conn);
}

void Engine::KillConnection(std::size_t i)
{
    LOG_D("Kill connection ", i);

    ASSERT(i < state_->conns.size());

    auto it = state_->conns.begin();
    it += i;
    (*it)->Disconnect(*state_);
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

        const auto batch_id = (*tit)->GetBatchId();

        (*tit)->Kill(*state_);
        state_->tasks.erase(tit);

        auto bit = std::find_if(std::begin(state_->batches), std::end(state_->batches),
            [&](const std::unique_ptr<BatchState>& b) {
                return b->id() == batch_id;
            });
        ASSERT(bit != std::end(state_->batches));

        auto& batch = *bit;
        if (!batch->HasTasks(*state_))
        {
            state_->batches.erase(bit);
        }
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

        const auto transition = task->Pause(*state_);
        if (transition)
        {
            auto* batch = state_->find_batch(task->GetBatchId());
            batch->UpdateState(*state_, *task, transition);
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

        const auto transition = task->Resume(*state_);
        if (transition)
        {
            auto* batch = state_->find_batch(task->GetBatchId());
            batch->UpdateState(*state_, *task, transition);
        }

    }
    state_->execute();

    return resumed;
}

void Engine::MoveTaskUp(std::size_t index)
{
    if (state_->group_items)
    {
        if (index > 0 && index < state_->batches.size()
            && state_->batches.size() > 1)
        {
            std::swap(state_->batches[index], state_->batches[index-1]);
            state_->repartition_task_list = true;
        }
    }
    else
    {
        if (index > 0 && index < state_->tasks.size()
            && state_->tasks.size() > 1)
        {
            std::swap(state_->tasks[index], state_->tasks[index - 1]);
        }
    }
}

void Engine::MoveTaskDown(std::size_t index)
{
    if (state_->group_items)
    {
        if (state_->batches.size() > 1
            && index < state_->batches.size() - 1)
        {
            std::swap(state_->batches[index], state_->batches[index + 1]);
            state_->repartition_task_list = true;
        }
    }
    else
    {
        if (state_->tasks.size() > 1
            && index < state_->tasks.size() - 1)
        {
            std::swap(state_->tasks[index], state_->tasks[index + 1]);
        }
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
    if (state_->group_items)
        return state_->batches.size();

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

    find_filetype("foo");

    openssl_init();

    identify_encoding("", 0);
}

} // newsflash
