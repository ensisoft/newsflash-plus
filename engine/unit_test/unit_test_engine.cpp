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
#  include <boost/test/minimal.hpp>
#include "newsflash/warnpop.h"

#include "engine/ui/account.h"
#include "engine/ui/task.h"
#include "engine/engine.h"
#include "engine/download.h"
#include "engine/connection.h"

using namespace newsflash;

class TestFileTask : public ContentTask
{
public:
   ~TestFileTask()
    {
    }

    virtual std::shared_ptr<CmdList> CreateCommands() override
    {
        std::shared_ptr<CmdList> ret;

        return ret;
    }

    virtual void Cancel() override
    {
        BOOST_REQUIRE(!Cancelled);
        BOOST_REQUIRE(!Committed);

        Cancelled = true;

    }

    virtual void Commit() override
    {
        BOOST_REQUIRE(!Cancelled);
        BOOST_REQUIRE(!Committed);

        Committed = true;
    }

    virtual void Complete(action& act,
        std::vector<std::unique_ptr<action>>& next) override
    {}

    virtual void Complete(CmdList& cmd,
        std::vector<std::unique_ptr<action>>& next) override
    {}

    virtual void Configure(const Settings& settings) override
    {}

    virtual bool HasCommands() const override
    {
        return true;
    }

    virtual std::size_t MaxNumActions() const override
    {
        return 1;
    }
    virtual bitflag<Error> GetErrors() const override
    {
        return bitflag<Error>();
    }

    virtual void Pack(data::TaskState& data) const override
    {}

    virtual void Load(const data::TaskState& data) override
    {}

    virtual void SetWriteCallback(const OnWriteDone& callback)
    {}

    bool Cancelled = false;
    bool Committed = false;
};

class TestHeadersTask : public ContentTask
{
public:
   ~TestHeadersTask()
    {
    }

    virtual std::shared_ptr<CmdList> CreateCommands() override
    {
        std::shared_ptr<CmdList> ret;

        return ret;
    }

    virtual void Cancel() override
    {
        BOOST_REQUIRE(!Cancelled);
        BOOST_REQUIRE(!Committed);

        Cancelled = true;

    }

    virtual void Commit() override
    {
        BOOST_REQUIRE(!Cancelled);
        BOOST_REQUIRE(!Committed);

        Committed = true;
    }

    virtual void Complete(action& act,
        std::vector<std::unique_ptr<action>>& next) override
    {}

    virtual void Complete(CmdList& cmd,
        std::vector<std::unique_ptr<action>>& next) override
    {}

    virtual void Configure(const Settings& settings) override
    {}

    virtual bool HasCommands() const override
    {
        return true;
    }

    virtual std::size_t MaxNumActions() const override
    {
        return 1;
    }
    virtual bitflag<Error> GetErrors() const override
    {
        return bitflag<Error>();
    }

    virtual void Pack(data::TaskState& data) const override
    {}

    virtual void Load(const data::TaskState& data) override
    {}

    virtual void SetWriteCallback(const OnWriteDone& callback)
    {}

    bool Cancelled = false;
    bool Committed = false;
};


class TestListingTask : public ContentTask
{
public:
   ~TestListingTask()
    {
    }

    virtual std::shared_ptr<CmdList> CreateCommands() override
    {
        std::shared_ptr<CmdList> ret;

        return ret;
    }

    virtual void Cancel() override
    {
        BOOST_REQUIRE(!Cancelled);
        BOOST_REQUIRE(!Committed);

        Cancelled = true;

    }

    virtual void Commit() override
    {
        BOOST_REQUIRE(!Cancelled);
        BOOST_REQUIRE(!Committed);

        Committed = true;
    }

    virtual void Complete(action& act,
        std::vector<std::unique_ptr<action>>& next) override
    {}

    virtual void Complete(CmdList& cmd,
        std::vector<std::unique_ptr<action>>& next) override
    {}

    virtual void Configure(const Settings& settings) override
    {}

    virtual bool HasCommands() const override
    {
        return true;
    }

    virtual std::size_t MaxNumActions() const override
    {
        return 1;
    }
    virtual bitflag<Error> GetErrors() const override
    {
        return bitflag<Error>();
    }

    virtual void Pack(data::TaskState& data) const override
    {}

    virtual void Load(const data::TaskState& data) override
    {}

    virtual void SetWriteCallback(const OnWriteDone& callback)
    {}

    bool Cancelled = false;
    bool Committed = false;
};

class TestConnection : public Connection
{
public:
    struct Resolve : public action
    {
        virtual void xperform() override
        {}
    };

    struct Connect : public action
    {
        virtual void xperform() override
        {}
    };

    struct Initialize : public action
    {
        virtual void xperform() override
        {}
    };

    struct Disconnect : public action
    {
        virtual void xperform() override
        {}
    };

    struct Ping : public action
    {
        virtual void xperform() override
        {}
    };

    struct Execute : public action
    {
        virtual void xperform() override
        {}
    };

    TestConnection()
    {
        std::memset(errors_, 0, sizeof(errors_));
    }

    void SetError(Connection::State when, Connection::Error what)
    {
        errors_[(int)when] = what;
    }

    virtual std::unique_ptr<action> Connect(const HostDetails& host) override
    {
        state_ = Connection::State::Resolving;
        return std::make_unique<Resolve>();
    }

    virtual std::unique_ptr<action> Disconnect() override
    {
        return nullptr;
    }

    virtual std::unique_ptr<action> Ping() override
    {
        return nullptr;
    }

    virtual std::unique_ptr<action> Complete(std::unique_ptr<action> a) override
    {
        auto* ptr = a.get();

        std::unique_ptr<action> next;

        if (errors_[(int)state_] != Connection::Error::None)
        {
            error_ = errors_[(int)state_];
            state_ = Connection::State::Error;
            return next;
        }

        if (dynamic_cast<Resolve*>(ptr))
        {
            next.reset(new struct Connect);
            state_ = Connection::State::Connecting;
        }
        else if (dynamic_cast<struct Connect*>(ptr))
        {
            next.reset(new Initialize());
            state_ = Connection::State::Initializing;
        }
        else if (dynamic_cast<struct Initialize*>(ptr))
        {
            state_ = Connection::State::Connected;
        }
        else if (dynamic_cast<struct Disconnect*>(ptr))
        {
            state_ = Connection::State::Disconnected;
        }
        else if (dynamic_cast<struct Execute*>(ptr))
        {
            state_ = Connection::State::Connected;
        }
        return next;
    }

    virtual std::unique_ptr<action> Execute(std::shared_ptr<CmdList> cmd, std::size_t tid) override
    {
        return nullptr;
    }

    virtual void Cancel() override
    {
    }

    virtual std::string GetUsername() const
    {
        return "user";
    }

    virtual std::string GetPassword() const override
    {
        return "pass";
    }

    virtual std::uint32_t GetCurrentSpeedBps() const override
    {
        return 0;
    }

    virtual std::uint64_t GetNumBytesTransferred() const override
    {
        return 0;
    }

    virtual State GetState() const override
    {
        return state_;
    }

    virtual Error GetError() const override
    {
        return error_;
    }
    virtual void SetCallback(const OnCmdlistDone& callback) override
    {}
private:
    Connection::State state_ = Connection::State::Disconnected;
    Connection::Error error_ = Connection::Error::None;
    Connection::Error errors_[7];

};

class ConnShell : public Connection
{
public:
    ConnShell(const std::shared_ptr<Connection>& conn) : conn_(conn)
    {}

    virtual std::unique_ptr<action> Connect(const HostDetails& host) override
    { return conn_->Connect(host);}

    virtual std::unique_ptr<action> Disconnect() override
    { return conn_->Disconnect(); }

    virtual std::unique_ptr<action> Ping() override
    { return conn_->Ping(); }

    virtual std::unique_ptr<action> Complete(std::unique_ptr<action> a) override
    { return conn_->Complete(std::move(a)); }

    virtual std::unique_ptr<action> Execute(std::shared_ptr<CmdList> cmd, std::size_t tid) override
    { return conn_->Execute(cmd, tid); }

    virtual void Cancel() override
    { conn_->Cancel(); }

    virtual std::string GetUsername() const
    { return conn_->GetUsername(); }

    virtual std::string GetPassword() const override
    { return conn_->GetPassword(); }

    virtual std::uint32_t GetCurrentSpeedBps() const override
    { return conn_->GetCurrentSpeedBps(); }

    virtual std::uint64_t GetNumBytesTransferred() const override
    { return conn_->GetNumBytesTransferred(); }

    virtual State GetState() const override
    { return conn_->GetState(); }

    virtual Error GetError() const override
    { return conn_->GetError(); }

    virtual void SetCallback(const OnCmdlistDone& callback) override
    { conn_->SetCallback(callback); }

private:
    std::shared_ptr<Connection> conn_;
};

class TaskShell : public ContentTask
{
public:
    TaskShell(std::shared_ptr<Task> task) : task_(task)
    {}
    virtual std::shared_ptr<CmdList> CreateCommands() override
    { return task_->CreateCommands(); }

    virtual void Cancel() override
    { task_->Cancel(); }

    virtual void Commit() override
    { task_->Commit(); }

    virtual void Complete(action& act,
        std::vector<std::unique_ptr<action>>& next) override
    { task_->Complete(act, next); }

    virtual void Complete(CmdList& cmd,
        std::vector<std::unique_ptr<action>>& next) override
    { task_->Complete(cmd, next); }

    virtual void Configure(const Settings& settings) override
    { task_->Configure(settings); }

    virtual bool HasCommands() const override
    { return task_->HasCommands(); }

    virtual std::size_t MaxNumActions() const override
    { return task_->MaxNumActions(); }

    virtual bitflag<Error> GetErrors() const override
    { return task_->GetErrors(); }

    virtual void Pack(data::TaskState& data) const override
    { task_->Pack(data); }

    virtual void Load(const data::TaskState& data) override
    { task_->Load(data); }

    virtual void SetWriteCallback(const OnWriteDone& callback)
    {}
private:
    std::shared_ptr<Task> task_;
};

struct Factory : public Engine::Factory
{
    std::shared_ptr<TestConnection> NextConn;

    virtual std::unique_ptr<Task> AllocateTask(const ui::FileDownload& file) override
    {
        auto task = std::make_shared<TestFileTask>();
        tasks_.push_back(task);
        return std::make_unique<TaskShell>(task);
    }
    virtual std::unique_ptr<Task> AllocateTask(const ui::HeaderDownload& download) override
    {
        auto task = std::make_shared<TestHeadersTask>();
        tasks_.push_back(task);
        return std::make_unique<TaskShell>(task);
    }
    virtual std::unique_ptr<Task> AllocateTask(const ui::GroupListDownload& list) override
    {
        auto task = std::make_shared<TestListingTask>();
        tasks_.push_back(task);
        return std::make_unique<TaskShell>(task);
    }
    virtual std::unique_ptr<Task> AllocateTask(const data::TaskState& data) override
    {
        auto task = std::make_shared<TestFileTask>();
        tasks_.push_back(task);
        return std::make_unique<TaskShell>(task);
    }
    virtual std::unique_ptr<Connection> AllocateConnection() override
    {
        std::shared_ptr<TestConnection> ret;
        if (NextConn)
            ret = NextConn;
        else ret = std::make_shared<TestConnection>();
        conns_.push_back(ret);
        NextConn.reset();
        return std::make_unique<ConnShell>(ret);
    }
    virtual std::unique_ptr<ui::Result> MakeResult(const Task& task, const ui::TaskDesc& desc) const override
    {
        return nullptr;
    }
    virtual std::unique_ptr<Logger> AllocateEngineLogger()
    {
        return std::make_unique<StdLogger>(std::cout);
    }
    virtual std::unique_ptr<Logger> AllocateConnectionLogger()
    {
        return std::make_unique<NullLogger>();
    }

    const Task* GetTask(size_t i) const
    {
        BOOST_REQUIRE(i < tasks_.size());
        return tasks_[i].get();
    }

    template<typename T>
    const T* GetTask(size_t i) const
    {
        BOOST_REQUIRE(i < tasks_.size());
        const auto* p = dynamic_cast<const T*>(tasks_[i].get());
        BOOST_REQUIRE(p);
        return p;
    }

private:
    std::vector<std::shared_ptr<Task>> tasks_;
    std::vector<std::shared_ptr<Connection>> conns_;
};

class FactoryShell : public Engine::Factory
{
public:
    FactoryShell(std::shared_ptr<Factory> factory) : factory_(factory)
    {}

    virtual std::unique_ptr<Task> AllocateTask(const ui::FileDownload& file) override
    { return factory_->AllocateTask(file); }

    virtual std::unique_ptr<Task> AllocateTask(const ui::HeaderDownload& download) override
    { return factory_->AllocateTask(download); }

    virtual std::unique_ptr<Task> AllocateTask(const ui::GroupListDownload& list) override
    { return factory_->AllocateTask(list); }

    virtual std::unique_ptr<Task> AllocateTask(const data::TaskState& data) override
    { return factory_->AllocateTask(data); }

    virtual std::unique_ptr<Connection> AllocateConnection() override
    { return factory_->AllocateConnection(); }

    virtual std::unique_ptr<ui::Result> MakeResult(const Task& task, const ui::TaskDesc& desc) const override
    { return factory_->MakeResult(task, desc); }

    virtual std::unique_ptr<Logger> AllocateEngineLogger()
    { return factory_->AllocateEngineLogger(); }

    virtual std::unique_ptr<Logger> AllocateConnectionLogger()
    { return factory_->AllocateConnectionLogger(); }

private:
    std::shared_ptr<Factory> factory_;
};

void test_connection_establish()
{
    struct TestCase {
        ui::Connection::Errors error = ui::Connection::Errors::None;
        Connection::Error what = Connection::Error::None;
        Connection::State when = Connection::State::Connecting;
    };
    TestCase tests[8];
    // tests[0] is a success case
    tests[0].what  = Connection::Error::None;
    tests[0].when  = Connection::State::Resolving;
    tests[0].error = ui::Connection::Errors::None;

    tests[1].what  = Connection::Error::Resolve;
    tests[1].when  = Connection::State::Resolving;
    tests[1].error = ui::Connection::Errors::Resolve;

    tests[2].what  = Connection::Error::Refused;
    tests[2].when  = Connection::State::Connecting;
    tests[2].error = ui::Connection::Errors::Refused;

    tests[3].what  = Connection::Error::Timeout;
    tests[3].when  = Connection::State::Connecting;
    tests[3].error = ui::Connection::Errors::Timeout;

    tests[4].what  = Connection::Error::Network;
    tests[4].when  = Connection::State::Connecting;
    tests[4].error = ui::Connection::Errors::Network;

    tests[5].what  = Connection::Error::Protocol;
    tests[5].when  = Connection::State::Initializing;
    tests[5].error = ui::Connection::Errors::Other;

    tests[6].what  = Connection::Error::PermissionDenied;
    tests[6].when  = Connection::State::Initializing;
    tests[6].error = ui::Connection::Errors::NoPermission;

    tests[7].what  = Connection::Error::Reset;
    tests[7].when  = Connection::State::Initializing;
    tests[7].error = ui::Connection::Errors::Other;

    for (size_t i=0; i < 8; ++i)
    {
        ui::Account account;
        account.id = 123;
        account.name = "test";
        account.username = "user";
        account.password = "pass";
        account.secure_host = "test.host.com";
        account.secure_port = 1000;
        account.connections = 1;
        account.enable_secure_server = true;
        account.enable_general_server = false;
        account.enable_compression = false;
        account.enable_pipelining = false;

        auto factory = std::make_shared<Factory>();
        factory->NextConn = std::make_shared<TestConnection>();
        factory->NextConn->SetError(tests[i].when, tests[i].what);

        const bool spawn_immediately = true;
        const bool debug_single_thread = true;
        Engine eng(std::make_unique<FactoryShell>(factory), debug_single_thread);

        std::deque<ui::Connection> conns;

        eng.Start();
        eng.SetAccount(account, spawn_immediately);

        // note that the the initial state is Resolving.
        const ui::Connection::States ExpectedStates[4] = {
            ui::Connection::States::Resolving,
            ui::Connection::States::Connecting,
            ui::Connection::States::Initializing,
            ui::Connection::States::Connected,
        };

        for (int j=0; j<4; ++j)
        {
            eng.GetConns(&conns);
            BOOST_REQUIRE(conns.size() == 1);
            if (conns[0].state != ExpectedStates[j])
            {
                // if there's an error we check that the error is
                // what the test case expected to fail
                BOOST_REQUIRE(conns[0].error == tests[i].error);
            }
            BOOST_REQUIRE(conns[0].id != 0);
            BOOST_REQUIRE(conns[0].task == 0);
            BOOST_REQUIRE(conns[0].account == 123);
            BOOST_REQUIRE(conns[0].down == 0);
            BOOST_REQUIRE(conns[0].host == "test.host.com");
            BOOST_REQUIRE(conns[0].port == 1000);
            BOOST_REQUIRE(conns[0].desc == "");
            BOOST_REQUIRE(conns[0].secure == true);
            BOOST_REQUIRE(conns[0].bps == 0);

            eng.RunMainThread();
            eng.Pump();
            if (conns[0].error != ui::Connection::Errors::None)
                break;
        }
        eng.KillConnection(0);
        eng.Stop();
        do
        {
            eng.RunMainThread();
        }
        while (eng.Pump());
        eng.GetConns(&conns);
        BOOST_REQUIRE(conns.empty());
    }

}

void test_task_entry_and_delete()
{

    auto factory = std::make_shared<Factory>();

    Engine eng(std::make_unique<FactoryShell>(factory), true);

    ui::GroupListDownload listing;
    listing.account = 123;
    listing.size    = 321;
    listing.path    = "test/foo/bar";
    listing.desc    = "test test";
    eng.SetGroupItems(false);
    eng.DownloadListing(listing);

    BOOST_REQUIRE(eng.GetNumTasks() == 1);
    ui::TaskDesc task;
    eng.GetTask(0, &task);
    BOOST_REQUIRE(task.error.any_bit() == false);
    BOOST_REQUIRE(task.state == ui::TaskDesc::States::Queued);
    BOOST_REQUIRE(task.task_id != 0);
    BOOST_REQUIRE(task.batch_id != 0);
    BOOST_REQUIRE(task.account == 123);
    BOOST_REQUIRE(task.desc == "test test");
    BOOST_REQUIRE(task.path == "test/foo/bar");
    BOOST_REQUIRE(task.size == 321);
    BOOST_REQUIRE(task.runtime == 0);
    BOOST_REQUIRE(task.etatime == 0);
    eng.KillTask(0);
    BOOST_REQUIRE(eng.GetNumTasks() == 0);
    BOOST_REQUIRE(factory->GetTask<TestListingTask>(0)->Cancelled == true);

    ui::FileDownload download;
    download.account = 123;
    download.size    = 666;
    download.path    = "test/foo/bar";
    download.desc    = "download";
    download.articles.push_back("1");
    download.articles.push_back("2");
    download.groups.push_back("alt.binaries.foo");
    ui::FileBatchDownload batch;
    batch.account = 123;
    batch.size    = 666;
    batch.path    = "test/foo/bar";
    batch.desc    = "download";
    batch.files.push_back(download);
    eng.DownloadFiles(batch);

    BOOST_REQUIRE(eng.GetNumTasks() == 1);
    eng.GetTask(0, &task);
    BOOST_REQUIRE(task.error.any_bit() == false);
    BOOST_REQUIRE(task.state == ui::TaskDesc::States::Queued);
    BOOST_REQUIRE(task.task_id != 0);
    BOOST_REQUIRE(task.batch_id != 0);
    BOOST_REQUIRE(task.account == 123);
    BOOST_REQUIRE(task.desc == "download");
    BOOST_REQUIRE(task.path == "test/foo/bar");
    BOOST_REQUIRE(task.size == 666);
    BOOST_REQUIRE(task.runtime == 0);
    BOOST_REQUIRE(task.etatime == 0);
    eng.KillTask(0);
    BOOST_REQUIRE(eng.GetNumTasks() == 0);
    BOOST_REQUIRE(factory->GetTask<TestFileTask>(1)->Cancelled == true);

}

void test_task_move()
{
    auto factory = std::make_shared<Factory>();

    Engine eng(std::make_unique<FactoryShell>(factory), true);
    eng.SetGroupItems(false);

    ui::FileBatchDownload batch;
    batch.account = 123;
    batch.size    = 666;
    batch.path    = "test/foo/bar";
    batch.desc    = "batch 1";
    for (int i=0; i<3; ++i)
    {
        ui::FileDownload download;
        download.account = 123;
        download.size    = 666;
        download.path    = "test/foo/bar";
        download.desc    = "file " + std::to_string(i+1);
        download.articles.push_back("1");
        download.articles.push_back("2");
        download.groups.push_back("alt.binaries.foo");
        batch.files.push_back(download);
    }
    eng.DownloadFiles(batch);

    std::deque<ui::TaskDesc> tasks;
    {

        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 3);
        BOOST_REQUIRE(tasks[0].desc == "file 1");
        BOOST_REQUIRE(tasks[1].desc == "file 2");
        BOOST_REQUIRE(tasks[2].desc == "file 3");
    }

    // move top up, cannont move
    {
        eng.MoveTaskUp(0);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 3);
        BOOST_REQUIRE(tasks[0].desc == "file 1");
        BOOST_REQUIRE(tasks[1].desc == "file 2");
        BOOST_REQUIRE(tasks[2].desc == "file 3");
    }

    // move bottom down, cannot move
    {
        eng.MoveTaskDown(2);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 3);
        BOOST_REQUIRE(tasks[0].desc == "file 1");
        BOOST_REQUIRE(tasks[1].desc == "file 2");
        BOOST_REQUIRE(tasks[2].desc == "file 3");
    }

    // move middle task up
    {
        eng.MoveTaskUp(1);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 3);
        BOOST_REQUIRE(tasks[0].desc == "file 2");
        BOOST_REQUIRE(tasks[1].desc == "file 1");
        BOOST_REQUIRE(tasks[2].desc == "file 3");
    }

    // move middle task down
    {
        eng.MoveTaskDown(1);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 3);
        BOOST_REQUIRE(tasks[0].desc == "file 2");
        BOOST_REQUIRE(tasks[1].desc == "file 3");
        BOOST_REQUIRE(tasks[2].desc == "file 1");
    }


    // move top down
    {
        eng.MoveTaskDown(0);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 3);
        BOOST_REQUIRE(tasks[0].desc == "file 3");
        BOOST_REQUIRE(tasks[1].desc == "file 2");
        BOOST_REQUIRE(tasks[2].desc == "file 1");
    }

    // move bottom up
    {
        eng.MoveTaskUp(2);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 3);
        BOOST_REQUIRE(tasks[0].desc == "file 3");
        BOOST_REQUIRE(tasks[1].desc == "file 1");
        BOOST_REQUIRE(tasks[2].desc == "file 2");
    }


    // add another batch
    batch.account = 444;
    batch.size    = 777;
    batch.path    = "test/foo/bar";
    batch.desc    = "batch 2";
    eng.DownloadFiles(batch);

    BOOST_REQUIRE(eng.GetNumTasks() == 6);
    eng.SetGroupItems(true);
    BOOST_REQUIRE(eng.GetNumTasks() == 2);

    // expected start state
    {

        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 2);
        BOOST_REQUIRE(tasks[0].desc == "batch 1");
        BOOST_REQUIRE(tasks[1].desc == "batch 2");
    }

    // move to up, cannot move
    {
        eng.MoveTaskUp(0);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 2);
        BOOST_REQUIRE(tasks[0].desc == "batch 1");
        BOOST_REQUIRE(tasks[1].desc == "batch 2");
    }
    // move bottom down, cannot move
    {
        eng.MoveTaskDown(1);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 2);
        BOOST_REQUIRE(tasks[0].desc == "batch 1");
        BOOST_REQUIRE(tasks[1].desc == "batch 2");
    }
    // move bottom up
    {
        eng.MoveTaskUp(1);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 2);
        BOOST_REQUIRE(tasks[0].desc == "batch 2");
        BOOST_REQUIRE(tasks[1].desc == "batch 1");
    }
    // move top down
    {
        eng.MoveTaskDown(0);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 2);
        BOOST_REQUIRE(tasks[0].desc == "batch 1");
        BOOST_REQUIRE(tasks[1].desc == "batch 2");
    }
}

void test_task_pause()
{}

void test_save_load_session()
{}






int test_main(int argc, char*[])
{
    newsflash::initialize();

    test_connection_establish();
    test_task_entry_and_delete();
    test_task_move();

    return 0;
}
