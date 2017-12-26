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

#include "test_minimal.h"

// use test_minimal instead because the boost.test is so retarded
// that it throws an exception when the test fails
// which unwinds the stack which then typically hits an assert in the engine's destructor.
#define BOOST_REQUIRE TEST_REQUIRE

#include <string>
#include <vector>
#include <iostream>

#include "engine/ui/account.h"
#include "engine/ui/task.h"
#include "engine/engine.h"
#include "engine/download.h"
#include "engine/connection.h"
#include "engine/session.h"
#include "engine/cmdlist.h"

using namespace newsflash;

struct TaskParams {
    bool should_cancel = false;
    bool should_commit = false;
    bool expect_cmdlist = false;
    size_t num_buffers = 0;
};

class TestFileTask : public ContentTask
{
public:
    TestFileTask()
    {}

    TestFileTask(const ui::FileDownload& file, const TaskParams& state) : state_(state)
    {
        articles_ = file.articles;
        groups_   = file.groups;
    }

   ~TestFileTask()
    {
        BOOST_REQUIRE(state_.should_cancel == cancelled_ ||
                      state_.should_commit == committed_);
        BOOST_REQUIRE(state_.expect_cmdlist == received_cmdlist_);
    }

    virtual std::shared_ptr<CmdList> CreateCommands() override
    {
        CmdList::Messages m;
        m.groups = groups_;
        m.numbers = articles_;
        articles_.clear();
        return std::make_shared<CmdList>(m);
    }

    virtual void Cancel() override
    {
        BOOST_REQUIRE(!cancelled_);
        BOOST_REQUIRE(!committed_);
        cancelled_ = true;
    }

    virtual void Commit() override
    {
        BOOST_REQUIRE(!cancelled_);
        BOOST_REQUIRE(!committed_);
        committed_ = true;
    }

    class DecodeJob : public action
    {
    public:
        DecodeJob(const std::string& cmd) : cmd_(cmd)
        {}

        virtual void xperform() override
        {
            if (cmd_ == "<throw_exception_on_decode>")
                throw std::runtime_error("some error");
        }
    private:
        friend class TestFileTask;
        std::string cmd_;
    };


    virtual void Complete(action& act,
        std::vector<std::unique_ptr<action>>& next) override
    {
        if (auto* job = dynamic_cast<DecodeJob*>(&act))
        {
            if (job->cmd_ == "<crc_mismatch>")
                errors_.set(Task::Error::CrcMismatch, true);
            else if (job->cmd_ == "<SizeMismatch>")
                errors_.set(Task::Error::SizeMismatch, true);
        }
    }

    virtual void Complete(CmdList& cmdlist,
        std::vector<std::unique_ptr<action>>& next) override
    {
        BOOST_REQUIRE(cmdlist.NumBuffers() == cmdlist.NumDataCommands());
        BOOST_REQUIRE(cmdlist.NumBuffers() == state_.num_buffers);

        for (size_t i=0; i<cmdlist.NumDataCommands(); ++i)
        {
            const auto& buff = cmdlist.GetBuffer(i);
            const auto& cmd  = cmdlist.GetCommand(i);
            const auto status = buff.GetContentStatus();
            if (cmd == "<none>")
            {
                BOOST_REQUIRE(buff.GetContentStatus() == Buffer::Status::None);
                articles_.push_back(cmd);
            }
            else if (cmd == "<success>")
            {
                BOOST_REQUIRE(buff.GetContentStatus() == Buffer::Status::Success);
                next.emplace_back(new DecodeJob(cmd));
            }
            else if (cmd == "<failure>")
            {
                BOOST_REQUIRE(buff.GetContentStatus() == Buffer::Status::Unavailable);
            }
            else if (cmd == "<dmca>")
            {
                BOOST_REQUIRE(buff.GetContentStatus() == Buffer::Status::Dmca);
            }
            else if (cmd == "<crc_mismatch>" ||
                    cmd == "<size_mismatch>" ||
                    cmd == "<throw_exception_on_decode>")
            {
                // some special messages to aid in the testing process.
                // these are considered succesful but have other properties.
                BOOST_REQUIRE(buff.GetContentStatus() == Buffer::Status::Success);
                next.emplace_back(new DecodeJob(cmd));
            }
            else if (cmd == "<maybe>")
            {
                BOOST_REQUIRE(status == Buffer::Status::Success ||
                    status == Buffer::Status::None);
                if (status == Buffer::Status::None)
                {
                    articles_.push_back(cmd);
                }

            }
        }
        received_cmdlist_ = true;
    }

    virtual void Configure(const Settings& settings) override
    {}

    virtual bool HasCommands() const override
    {
        return !articles_.empty();
    }
    virtual bitflag<Error> GetErrors() const override
    {
        return errors_;
    }
    virtual bool CanSerialize() const override
    { return true; }

    virtual void Pack(std::string* data) const override
    {
        std::stringstream ss;
        ss << std::to_string(articles_.size()) << "\n";
        for (const auto& a : articles_)
            ss << a << "\n";
        ss << std::to_string(groups_.size()) << "\n";
        for (const auto& g : groups_)
            ss << g << "\n";

        std::string bytes;
        bytes.resize(sizeof(state_));
        std::memcpy(&bytes[0], &state_, sizeof(state_));
        ss << bytes;

        *data = ss.str();
    }

    virtual void Load(const std::string& data) override
    {
        std::stringstream ss(data);

        {
            std::string line;
            std::getline(ss, line);

            std::stringstream tmp(line);
            std::size_t num_articles = 0;
            tmp >> num_articles;
            for (size_t i=0; i<num_articles; ++i)
            {
                std::string a;
                std::getline(ss, a);
                articles_.push_back(a);
            }
        }

        {
            std::string line;
            std::getline(ss, line);

            std::stringstream tmp(line);
            std::size_t num_groups = 0;
            tmp >> num_groups;
            for (size_t i=0; i<num_groups; ++i)
            {
                std::string a;
                std::getline(ss, a);
                groups_.push_back(a);
            }
        }

        ss.read((char*)&state_, sizeof(state_));
    }

    virtual void SetWriteCallback(const OnWriteDone& callback)
    {}

private:
    bool cancelled_ = false;
    bool committed_ = false;
    bool received_cmdlist_ = false;
    bitflag<Error> errors_;
private:
    TaskParams state_;
private:
    std::vector<std::string> articles_;
    std::vector<std::string> groups_;
};

class TestHeadersTask : public ContentTask
{
public:
    TestHeadersTask(const TaskParams& params) : state_(params)
    {}

   ~TestHeadersTask()
    {
        BOOST_REQUIRE(state_.should_cancel == cancelled_ ||
                      state_.should_commit == committed_);
    }

    virtual std::shared_ptr<CmdList> CreateCommands() override
    {
        std::shared_ptr<CmdList> ret;

        return ret;
    }

    virtual void Cancel() override
    {
        BOOST_REQUIRE(!cancelled_);
        BOOST_REQUIRE(!committed_);

        cancelled_ = true;

    }

    virtual void Commit() override
    {
        BOOST_REQUIRE(!cancelled_);
        BOOST_REQUIRE(!committed_);

        committed_ = true;
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

    virtual bitflag<Error> GetErrors() const override
    {
        return bitflag<Error>();
    }

    virtual void SetWriteCallback(const OnWriteDone& callback)
    {}
private:
    bool cancelled_ = false;
    bool committed_ = false;
private:
    TaskParams state_;
};


class TestListingTask : public ContentTask
{
public:
    TestListingTask(const TaskParams& state) : state_(state)
    {}

   ~TestListingTask()
    {
        BOOST_REQUIRE(state_.should_cancel == cancelled_ ||
                      state_.should_commit == committed_);
    }

    virtual std::shared_ptr<CmdList> CreateCommands() override
    {
        std::shared_ptr<CmdList> ret;

        return ret;
    }

    virtual void Cancel() override
    {
        BOOST_REQUIRE(!cancelled_);
        BOOST_REQUIRE(!committed_);

        cancelled_ = true;
    }

    virtual void Commit() override
    {
        BOOST_REQUIRE(!cancelled_);
        BOOST_REQUIRE(!committed_);

        committed_ = true;
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

    virtual bitflag<Error> GetErrors() const override
    {
        return bitflag<Error>();
    }

    virtual void SetWriteCallback(const OnWriteDone& callback)
    {}
private:
    bool cancelled_ = false;
    bool committed_ = false;
private:
    TaskParams state_;
};

struct ConnState {
    Connection::Error errors_[7];

    bool force_no_such_group = false;
    bool force_no_such_body  = false;

    ConnState()
    {
        std::memset(&errors_, 0, sizeof(errors_));
    }

    void SetError(Connection::State when, Connection::Error what)
    {
        errors_[(int)when] = what;
    }
};

void set(Buffer& buff, const char* str)
{
    buff.Clear();
    std::strcpy(buff.Back(), str);
    buff.Append(std::strlen(str));
}

void append(Buffer& buff, const char* str)
{
    std::strcpy(buff.Back(), str);
    buff.Append(std::strlen(str));
}

class TestConnection : public Connection
{
public:
    struct Resolve : public action
    {
        virtual void xperform() override
        {
            if (host_ == "test.exception.com")
                throw std::runtime_error("surprise!");
        }
        std::string host_;
    };

    struct Connect : public action
    {
        virtual void xperform() override
        {}
    };

    struct Initialize : public action
    {
        Initialize(std::shared_ptr<Session> session) : session_(session)
        {}

        virtual void xperform() override
        {
            std::string command;
            session_->on_send = [&](const std::string& cmd) {
                command = cmd;
            };
            session_->on_auth = [](std::string& user, std::string& pass) {
                user = "user";
                pass = "pass";
            };

            const bool authenticate_immediately = false;
            session_->Start(authenticate_immediately);

            newsflash::Buffer incoming(1024);
            newsflash::Buffer tmp;

            // we don't really check any proper session errors
            // since we're faking error conditions from connection
            // instead we just setup the session so that it's in a "good state"
            // for later testing

            session_->SendNext();
            set(incoming, "200 welcome posting allowed\r\n");
            session_->RecvNext(incoming, tmp);
            session_->SendNext();
            set(incoming, "101 capabilities list follows\r\n"
                "MODE-READER\r\n"
                "XZVER\r\n"
                "IHAVE\r\n"
                "\r\n"
                ".\r\n");
            session_->RecvNext(incoming, tmp);
            session_->SendNext();
            set(incoming, "200 posting allowed\r\n");
            session_->RecvNext(incoming, tmp);
            BOOST_REQUIRE(session_->HasPending() == false);
            BOOST_REQUIRE(session_->GetError() == Session::Error::None);
        }

        std::shared_ptr<Session> session_;
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

    struct DummyExecute : public action
    {
        virtual void xperform() override
        {}

        virtual void run_completion_callbacks() override
        {
            Connection::CmdListCompletionData completion;
            completion.cmds          = cmdlist;
            completion.task_owner_id = taskid;
            completion.total_bytes   = 300;
            completion.content_bytes = 200;
            completion.execution_did_complete = !has_exception();
            callback(completion);
        }
        std::size_t taskid = 0;
        std::shared_ptr<CmdList> cmdlist;
        Connection::OnCmdlistDone callback;
    };

    struct Execute : public action
    {
        virtual void xperform() override
        {
            std::string command;
            session->on_send = [&](const std::string& out) {
                command = out;
            };
            session->on_auth = [](std::string& user, std::string& pass) {
                user = "user";
                pass = "pass";
            };

            if (cmdlist->NeedsToConfigure())
            {
                Buffer incoming(1024);
                Buffer out;

                cmdlist->SubmitConfigureCommand(0, *session);
                session->SendNext();
                if (command == "GROUP alt.binaries.success\r\n")
                {
                    if (force_no_such_group)
                    {
                        set(incoming, "411 no such group");
                    }
                    else
                    {
                        set(incoming, "211 4 1 4 alt.binaries.success ok\r\n");
                    }
                }
                else if (command == "GROUP alt.binaries.failure\r\n")
                {
                    set(incoming, "411 no such group");
                }
                session->RecvNext(incoming, out);
                cmdlist->ReceiveConfigureBuffer(0, out);
            }
            if (!cmdlist->IsGood())
            {
                return;
            }

            for (size_t i=0; i<cmdlist->NumDataCommands(); ++i)
            {
                Buffer incoming(1024);
                Buffer out;

                cmdlist->SubmitDataCommand(i, *session);
                session->SendNext();
                if (command == "BODY <success>\r\n")
                {
                    if (force_no_such_body)
                    {
                        set(incoming, "420 no such article\r\n");
                    }
                    else
                    {
                        set(incoming, "222 body follows\r\n"
                            "here's some content\r\n"
                            ".\r\n");
                    }
                }
                else if (command == "BODY <crc_mismatch>\r\n")
                {
                    if (force_no_such_body)
                    {
                        set(incoming, "420 no such article\r\n");
                    }
                    else
                    {
                        set(incoming, "222 body follows\r\n"
                            "crc_mismatch\r\n"
                            ".\r\n");
                    }
                }
                else if (command == "BODY <size_mismatch>\r\n")
                {
                    if (force_no_such_body)
                    {
                        set(incoming, "420 no such article\r\n");
                    }
                    else
                    {
                        set(incoming, "222 body follows\r\n"
                            "size_mismatch\r\n"
                            ".\r\n");
                    }
                }
                else if (command == "BODY <throw_exception_on_decode>\r\n")
                {
                    if (force_no_such_body)
                    {
                        set(incoming, "420 no such article\r\n");
                    }
                    else
                    {
                        set(incoming, "222 body follows\r\n"
                            "throw_exception_on_decode\r\n"
                            ".\r\n");
                    }
                }
                else if (command == "BODY <maybe>\r\n")
                {
                    if (force_no_such_body)
                    {
                        set(incoming, "420 no such article\r\n");
                    }
                    else
                    {
                        set(incoming, "222 body follows\r\n"
                            "maybe\r\n"
                            ".\r\n");
                    }
                }
                else if (command == "BODY <failure>\r\n")
                {
                    set(incoming, "420 no such article\r\n");
                }
                else if (command == "BODY <dmca>\r\n")
                {
                    set(incoming, "420 DMCA takedown\r\n");
                }
                session->RecvNext(incoming, out);
                cmdlist->ReceiveDataBuffer(std::move(out));
            }
        }
        virtual void run_completion_callbacks() override
        {
            Connection::CmdListCompletionData completion;
            completion.cmds          = cmdlist;
            completion.task_owner_id = taskid;
            completion.total_bytes   = 300;
            completion.content_bytes = 200;
            completion.execution_did_complete = !has_exception();
            callback(completion);
        }
        std::size_t taskid = 0;
        std::shared_ptr<CmdList> cmdlist;
        std::shared_ptr<Session> session;
        Connection::OnCmdlistDone callback;

        bool force_no_such_body = false;
        bool force_no_such_group = false;
    };

    TestConnection(const ConnState& state) : conn_state_(state)
    {
        session_ = std::make_shared<Session>();
    }

    virtual std::unique_ptr<action> Connect(const HostDetails& host) override
    {
        state_ = Connection::State::Resolving;
        session_->Reset();

        auto ret = std::make_unique<Resolve>();
        ret->host_ = host.hostname;
        return std::move(ret);
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

        if (conn_state_.errors_[(int)state_] != Connection::Error::None)
        {
            error_ = conn_state_.errors_[(int)state_];
            state_ = Connection::State::Error;
            return next;
        }

        if (auto* p = dynamic_cast<Resolve*>(ptr))
        {
            next.reset(new struct Connect);
            state_ = Connection::State::Connecting;
        }
        else if (dynamic_cast<struct Connect*>(ptr))
        {
            next.reset(new Initialize(session_));
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
        if (conn_state_.errors_[(int)Connection::State::Active] != Connection::Error::None)
        {
            auto ret = std::make_unique<struct DummyExecute>();
            ret->taskid = tid;
            ret->cmdlist = cmd;
            ret->callback = callback_;
            state_ = Connection::State::Active;
            return std::move(ret);
        }

        auto ret = std::make_unique<struct Execute>();
        ret->session = session_;
        ret->taskid = tid;
        ret->callback = callback_;
        ret->cmdlist = cmd;
        ret->force_no_such_body  = conn_state_.force_no_such_body;
        ret->force_no_such_group = conn_state_.force_no_such_group;
        state_ = Connection::State::Active;
        return std::move(ret);
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
    {
        callback_ = callback;
    }
private:
    Connection::State state_ = Connection::State::Disconnected;
    Connection::Error error_ = Connection::Error::None;
    ConnState conn_state_;

private:
    std::shared_ptr<Session> session_;
    OnCmdlistDone callback_;

};


struct Factory : public Engine::Factory
{

    virtual std::unique_ptr<Task> AllocateTask(const ui::FileDownload& file) override
    {
        auto* params = static_cast<TaskParams*>(file.user_data);

        return std::make_unique<TestFileTask>(file, *params);
    }

    virtual std::unique_ptr<Task> AllocateTask(const ui::HeaderDownload& download) override
    {
        auto* params = static_cast<TaskParams*>(download.user_data);

        return std::make_unique<TestHeadersTask>(*params);
    }
    virtual std::unique_ptr<Task> AllocateTask(const ui::GroupListDownload& list) override
    {
        auto* params = static_cast<TaskParams*>(list.user_data);

        return std::make_unique<TestListingTask>(*params);
    }
    virtual std::unique_ptr<Task> AllocateTask(std::size_t type) override
    {
        BOOST_REQUIRE(type == 1);

        return std::make_unique<TestFileTask>();
    }
    virtual std::unique_ptr<Connection> AllocateConnection(const ui::Account& acc) override
    {
        auto* params = static_cast<ConnState*>(acc.user_data);

        return std::make_unique<TestConnection>(*params);
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
private:

};

void test_connection_establish_success()
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
        ConnState state;
        state.SetError(tests[i].when, tests[i].what);

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
        account.user_data = &state;

        const bool spawn_immediately = true;
        const bool debug_single_thread = true;
        Engine eng(std::make_unique<Factory>(), debug_single_thread);

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

// test unexpected exception during connection establish
// expected: connection state is error
void test_connection_establish_failure()
{
    ConnState state;

    ui::Account account;
    account.id = 123;
    account.name = "test";
    account.username = "user";
    account.password = "pass";
    account.secure_host = "test.exception.com";
    account.secure_port = 1000;
    account.connections = 1;
    account.enable_secure_server = true;
    account.enable_general_server = false;
    account.enable_compression = false;
    account.enable_pipelining = false;
    account.user_data = &state;

    const bool spawn_immediately = true;
    const bool debug_single_thread = true;
    Engine eng(std::make_unique<Factory>(), debug_single_thread);

    std::deque<ui::Connection> conns;

    eng.Start();
    eng.SetAccount(account, spawn_immediately);

    eng.RunMainThread();
    eng.Pump();

    ui::Connection conn;
    eng.GetConn(0, &conn);
    BOOST_REQUIRE(conn.state == ui::Connection::States::Error);

    eng.KillConnection(0);
    eng.Stop();
    do
    {
        eng.RunMainThread();
        eng.Pump();
    }
    while (eng.HasPendingActions());
}

// test connection reconnect on several error conditions:
// expected result: connection is reconnected after a number of ticks.
// precondtion: test_connection_establish_success and test_connection_establish_failure pass
void test_connection_reconnect()
{
    struct TestCase {
        ui::Connection::Errors error = ui::Connection::Errors::None;
        Connection::Error what = Connection::Error::None;
        Connection::State when = Connection::State::Connecting;
        bool will_reconnect = true;
    };
    TestCase tests[7];


    // all are error cases
    tests[0].what  = Connection::Error::Resolve;
    tests[0].when  = Connection::State::Resolving;
    tests[0].error = ui::Connection::Errors::Resolve;

    tests[1].what  = Connection::Error::Refused;
    tests[1].when  = Connection::State::Connecting;
    tests[1].error = ui::Connection::Errors::Refused;

    tests[2].what  = Connection::Error::Timeout;
    tests[2].when  = Connection::State::Connecting;
    tests[2].error = ui::Connection::Errors::Timeout;

    tests[3].what  = Connection::Error::Network;
    tests[3].when  = Connection::State::Connecting;
    tests[3].error = ui::Connection::Errors::Network;

    tests[4].what  = Connection::Error::Protocol;
    tests[4].when  = Connection::State::Initializing;
    tests[4].error = ui::Connection::Errors::Other;

    tests[5].what  = Connection::Error::PermissionDenied;
    tests[5].when  = Connection::State::Initializing;
    tests[5].error = ui::Connection::Errors::NoPermission;
    tests[5].will_reconnect = false;

    tests[6].what  = Connection::Error::Reset;
    tests[6].when  = Connection::State::Initializing;
    tests[6].error = ui::Connection::Errors::Other;

    for (size_t i=0; i < 7; ++i)
    {
        ConnState state;
        state.SetError(tests[i].when, tests[i].what);

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
        account.user_data = &state;

        const bool spawn_immediately = true;
        const bool debug_single_thread = true;
        Engine eng(std::make_unique<Factory>(), debug_single_thread);

        eng.Start();
        eng.SetAccount(account, spawn_immediately);

        ui::Connection conn;
        do
        {
            eng.RunMainThread();
            eng.Pump();
            eng.GetConn(0, &conn);
        }
        while (conn.state != ui::Connection::States::Error);

        // todo: fix the tick parameters
        const auto TicksUntilReconnect = 5;

        for (int i=0; i<TicksUntilReconnect; ++i)
        {
            eng.Tick();
        }

        if (tests[i].will_reconnect)
        {
            eng.GetConn(0, &conn);
            BOOST_REQUIRE(conn.state == ui::Connection::States::Disconnected);

            eng.RunMainThread();
            eng.Pump();
            eng.GetConn(0, &conn);
            BOOST_REQUIRE(conn.state != ui::Connection::States::Disconnected);
        }
        else
        {
            eng.GetConn(0, &conn);
            BOOST_REQUIRE(conn.state == ui::Connection::States::Error);
        }

        eng.KillConnection(0);
        eng.Stop();
        do
        {
            eng.RunMainThread();
        }
        while (eng.Pump());
    }
}

void test_task_entry_and_delete()
{
    Engine eng(std::make_unique<Factory>(), true);

    {
        TaskParams params;
        params.should_cancel = true;

        ui::GroupListDownload listing;
        listing.account   = 123;
        listing.size      = 321;
        listing.path      = "test/foo/bar";
        listing.desc      = "test test";
        listing.user_data = &params;
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
    }

    {
        TaskParams params;
        params.should_cancel = true;

        ui::FileDownload download;
        download.account   = 123;
        download.size      = 666;
        download.path      = "test/foo/bar";
        download.desc      = "download";
        download.user_data = &params;
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

        ui::TaskDesc task;

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
    }
}

void test_task_move()
{
    Engine eng(std::make_unique<Factory>(), true);
    eng.SetGroupItems(false);

    const auto NumTestFiles = 3;

    std::vector<TaskParams> params;
    params.resize(NumTestFiles);

    ui::FileBatchDownload batch;
    batch.account = 123;
    batch.size    = 666;
    batch.path    = "test/foo/bar";
    batch.desc    = "batch 1";
    for (int i=0; i<NumTestFiles; ++i)
    {
        ui::FileDownload download;
        download.account = 123;
        download.size    = 666;
        download.path    = "test/foo/bar";
        download.desc    = "file " + std::to_string(i+1);
        download.articles.push_back("1");
        download.articles.push_back("2");
        download.groups.push_back("alt.binaries.foo");
        download.user_data = &params[i];

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

void test_task_execute_success()
{
    struct TestCase {
        std::vector<std::string> articles;
        std::vector<std::string> groups;
        bitflag<ui::TaskDesc::Errors> errors;
    };

    std::vector<TestCase> tests;

    tests.emplace_back();
    tests[0].articles.push_back("<success>");
    tests[0].articles.push_back("<success>");
    tests[0].groups.push_back("alt.binaries.success");

    tests.emplace_back();
    tests[1].articles.push_back("<success>");
    tests[1].articles.push_back("<dmca>");
    tests[1].groups.push_back("alt.binaries.success");
    tests[1].errors.set(ui::TaskDesc::Errors::Dmca, true);

    tests.emplace_back();
    tests[2].articles.push_back("<dmca>");
    tests[2].articles.push_back("<dmca>");
    tests[2].groups.push_back("alt.binaries.success");
    tests[2].errors.set(ui::TaskDesc::Errors::Dmca, true);
    tests[2].errors.set(ui::TaskDesc::Errors::Unavailable, true);

    tests.emplace_back();
    tests[3].articles.push_back("<failure>");
    tests[3].articles.push_back("<dmca>");
    tests[3].groups.push_back("alt.binaries.success");
    tests[3].errors.set(ui::TaskDesc::Errors::Dmca, true);
    tests[3].errors.set(ui::TaskDesc::Errors::Incomplete, true);
    tests[3].errors.set(ui::TaskDesc::Errors::Unavailable, true);

    tests.emplace_back();
    tests[4].articles.push_back("<success>");
    tests[4].articles.push_back("<crc_mismatch>");
    tests[4].groups.push_back("alt.binaries.success");
    tests[4].errors.set(ui::TaskDesc::Errors::Damaged, true);

    tests.emplace_back();
    tests[5].articles.push_back("<dmca>");
    tests[5].articles.push_back("<crc_mismatch>");
    tests[5].groups.push_back("alt.binaries.success");
    tests[5].errors.set(ui::TaskDesc::Errors::Damaged, true);
    tests[5].errors.set(ui::TaskDesc::Errors::Dmca, true);

    // no such group so all buffers are with None status
    // # issue 66
    tests.emplace_back();
    tests[6].articles.push_back("<failure>");
    tests[6].articles.push_back("<failure>");
    tests[6].groups.push_back("alt.binaries.failure");
    tests[6].errors.set(ui::TaskDesc::Errors::Unavailable, true);
    tests[6].errors.set(ui::TaskDesc::Errors::Incomplete, true);


    for (size_t i=0; i < tests.size(); ++i)
    {
        const bool spawn_immediately = true;
        const bool debug_single_thread = true;

        ConnState test_conn_params;

        Engine eng(std::make_unique<Factory>(), debug_single_thread);
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
        account.user_data = &test_conn_params;
        eng.Start();
        eng.SetAccount(account, spawn_immediately);
        do
        {
            eng.RunMainThread();
            eng.Pump();

            ui::Connection conn;
            eng.GetConn(0, &conn);
            if (conn.state == ui::Connection::States::Connected)
                break;

        } while(true);


        TaskParams params;
        params.should_commit = true;
        params.expect_cmdlist = true;
        params.num_buffers = 2;

        ui::FileDownload download;
        download.account   = 123;
        download.size      = 666;
        download.path      = "test/foo/bar";
        download.desc      = "download";
        download.articles  = tests[i].articles;
        download.groups    = tests[i].groups;
        download.user_data = &params;
        ui::FileBatchDownload batch;
        batch.account = 123;
        batch.size    = 666;
        batch.path    = "test/foo/bar";
        batch.desc    = "download";
        batch.files.push_back(download);
        eng.DownloadFiles(batch);

        ui::TaskDesc task;
        eng.GetTask(0, &task);
        BOOST_REQUIRE(task.state == ui::TaskDesc::States::Active);

        ui::Connection conn;
        eng.GetConn(0, &conn);
        BOOST_REQUIRE(conn.state == ui::Connection::States::Active);
        BOOST_REQUIRE(conn.task == task.task_id);

        do
        {
            eng.RunMainThread();
            eng.Pump();
        }
        while (eng.HasPendingActions());

        eng.GetConn(0, &conn);
        BOOST_REQUIRE(conn.state == ui::Connection::States::Connected);
        BOOST_REQUIRE(conn.task == 0);

        eng.GetTask(0, &task);
        //BOOST_REQUIRE(task.state == tests[i].result_state);
        BOOST_REQUIRE(task.state == ui::TaskDesc::States::Complete);
        BOOST_REQUIRE(task.error == tests[i].errors);

        eng.KillConnection(0);
        eng.Stop();
        do
        {
            eng.RunMainThread();
            eng.Pump();
        }
        while (eng.HasPendingActions());
    }
}

// unexpected failure, i.e. an exception happens.
// expected: task goes into error state.
void test_task_execute_failure()
{
    const bool spawn_immediately = true;
    const bool debug_single_thread = true;

    ConnState test_conn_params;

    Engine eng(std::make_unique<Factory>(), debug_single_thread);
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
    account.user_data = &test_conn_params;
    eng.Start();
    eng.SetAccount(account, spawn_immediately);
    do
    {
        eng.RunMainThread();
        eng.Pump();

        ui::Connection conn;
        eng.GetConn(0, &conn);
        if (conn.state == ui::Connection::States::Connected)
            break;

    } while(true);


    TaskParams params;
    params.should_commit = false;
    params.expect_cmdlist = true;
    params.should_cancel = true;
    params.num_buffers = 1;

    ui::FileDownload download;
    download.account   = 123;
    download.size      = 666;
    download.path      = "test/foo/bar";
    download.desc      = "download";
    download.articles.push_back("<throw_exception_on_decode>");
    download.groups.push_back("alt.binaries.success");
    download.user_data = &params;
    ui::FileBatchDownload batch;
    batch.account = 123;
    batch.size    = 666;
    batch.path    = "test/foo/bar";
    batch.desc    = "download";
    batch.files.push_back(download);
    eng.DownloadFiles(batch);

    // execute the execute action.
    eng.RunMainThread();
    eng.Pump();

    // run the decode job.
    eng.RunMainThread();
    eng.Pump();

    ui::TaskDesc task;
    eng.GetTask(0, &task);
    BOOST_REQUIRE(task.state == ui::TaskDesc::States::Error);

    eng.KillConnection(0);
    eng.Stop();
    do
    {
        eng.RunMainThread();
        eng.Pump();
    }
    while (eng.HasPendingActions());
}

void test_task_execute_restart()
{
    const bool spawn_immediately = true;
    const bool debug_single_thread = true;

    Engine eng(std::make_unique<Factory>(), debug_single_thread);

    std::unique_ptr<ConnState> initial_params(new ConnState);
    initial_params->SetError(Connection::State::Active, Connection::Error::PermissionDenied);

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
        account.user_data = initial_params.get();
        eng.SetAccount(account, false);
    }

    TaskParams params;
    params.should_commit = true;
    params.expect_cmdlist = true;
    params.num_buffers = 2;

    ui::FileDownload download;
    download.account   = 123;
    download.size      = 666;
    download.path      = "test/foo/bar";
    download.desc      = "download";
    download.articles.push_back("<maybe>");
    download.articles.push_back("<maybe>");
    download.groups.push_back("alt.binaries.success");
    download.user_data = &params;
    ui::FileBatchDownload batch;
    batch.account = 123;
    batch.size    = 666;
    batch.path    = "test/foo/bar";
    batch.desc    = "download";
    batch.files.push_back(download);
    eng.DownloadFiles(batch);
    eng.Start();

    while (eng.HasPendingActions())
    {
        eng.RunMainThread();
        eng.Pump();
    }

    ui::Connection conn;
    eng.GetConn(0, &conn);
    BOOST_REQUIRE(conn.state == ui::Connection::States::Error);
    BOOST_REQUIRE(conn.error == ui::Connection::Errors::NoPermission);

    ui::TaskDesc task;
    eng.GetTask(0, &task);
    BOOST_REQUIRE(task.state == ui::TaskDesc::States::Waiting);

    // reset the connection
    std::unique_ptr<ConnState> test_conn_params(new ConnState);
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
        account.user_data = test_conn_params.get();
        eng.SetAccount(account, false);
    }

    eng.Stop();
    eng.Start();

    while (eng.HasPendingActions())
    {
        eng.RunMainThread();
        eng.Pump();
    }

    eng.GetConn(0, &conn);
    BOOST_REQUIRE(conn.state == ui::Connection::States::Connected);

    eng.GetTask(0, &task),
    BOOST_REQUIRE(task.state == ui::TaskDesc::States::Complete);

    eng.KillTask(0);
    eng.KillConnection(0);
    eng.Stop();

    while (eng.HasPendingActions())
    {
        eng.RunMainThread();
        eng.Pump();
    }

}

// test that fill server account works.
// expected: task is filled from the fill server account.
void test_task_execute_fill_success()
{
    struct TestCase {
        std::vector<std::string> articles;
        std::vector<std::string> groups;
        bitflag<ui::TaskDesc::Errors> errors;
    };

    std::vector<TestCase> tests;

    tests.emplace_back();
    tests[0].articles.push_back("<success>");
    tests[0].articles.push_back("<success>");
    tests[0].groups.push_back("alt.binaries.success");

    tests.emplace_back();
    tests[1].articles.push_back("<success>");
    tests[1].articles.push_back("<dmca>");
    tests[1].groups.push_back("alt.binaries.success");
    tests[1].errors.set(ui::TaskDesc::Errors::Dmca, true);

    tests.emplace_back();
    tests[2].articles.push_back("<dmca>");
    tests[2].articles.push_back("<dmca>");
    tests[2].groups.push_back("alt.binaries.success");
    tests[2].errors.set(ui::TaskDesc::Errors::Dmca, true);
    tests[2].errors.set(ui::TaskDesc::Errors::Unavailable, true);

    tests.emplace_back();
    tests[3].articles.push_back("<failure>");
    tests[3].articles.push_back("<dmca>");
    tests[3].groups.push_back("alt.binaries.success");
    tests[3].errors.set(ui::TaskDesc::Errors::Dmca, true);
    tests[3].errors.set(ui::TaskDesc::Errors::Incomplete, true);
    tests[3].errors.set(ui::TaskDesc::Errors::Unavailable, true);

    tests.emplace_back();
    tests[4].articles.push_back("<success>");
    tests[4].articles.push_back("<crc_mismatch>");
    tests[4].groups.push_back("alt.binaries.success");
    tests[4].errors.set(ui::TaskDesc::Errors::Damaged, true);

    tests.emplace_back();
    tests[5].articles.push_back("<dmca>");
    tests[5].articles.push_back("<crc_mismatch>");
    tests[5].groups.push_back("alt.binaries.success");
    tests[5].errors.set(ui::TaskDesc::Errors::Damaged, true);
    tests[5].errors.set(ui::TaskDesc::Errors::Dmca, true);

    // no such group so all buffers are with None status
    // # issue 66
    tests.emplace_back();
    tests[6].articles.push_back("<failure>");
    tests[6].articles.push_back("<failure>");
    tests[6].groups.push_back("alt.binaries.failure");
    tests[6].errors.set(ui::TaskDesc::Errors::Unavailable, true);
    tests[6].errors.set(ui::TaskDesc::Errors::Incomplete, true);

    for (size_t i=0; i < tests.size(); ++i)
    {
        const bool spawn_immediately   = true;
        const bool debug_single_thread = true;

        Engine eng(std::make_unique<Factory>(), debug_single_thread);

        ConnState test_main_params;
        ConnState test_fill_params;
        test_main_params.force_no_such_body = true;

        ui::Account main_account;
        main_account.id = 123;
        main_account.name = "main";
        main_account.username = "user";
        main_account.password = "pass";
        main_account.secure_host = "test.main.com";
        main_account.secure_port = 1000;
        main_account.connections = 1;
        main_account.enable_secure_server = true;
        main_account.enable_general_server = false;
        main_account.enable_compression = false;
        main_account.enable_pipelining = false;
        main_account.user_data = &test_main_params;

        ui::Account fill_account;
        fill_account.id = 321;
        fill_account.name = "fill";
        fill_account.username = "user";
        fill_account.password = "pass";
        fill_account.secure_host = "test.fill.com";
        fill_account.secure_port = 1000;
        fill_account.connections = 1;
        fill_account.enable_secure_server = true;
        fill_account.enable_general_server = false;
        fill_account.enable_compression = false;
        fill_account.enable_pipelining = false;
        fill_account.user_data = &test_fill_params;

        eng.SetAccount(main_account, true);
        eng.SetAccount(fill_account, false);
        eng.SetFillAccount(321);
        eng.Start();

        do
        {
            eng.RunMainThread();
            eng.Pump();

            ui::Connection conn;
            eng.GetConn(0, &conn);
            if (conn.state == ui::Connection::States::Connected)
                break;
        }
        while (true);

        TaskParams params;
        params.should_commit = true;
        params.expect_cmdlist = true;
        params.num_buffers = 2;

        ui::FileDownload download;
        download.account   = 123;
        download.size      = 666;
        download.path      = "test/foo/bar";
        download.desc      = "download";
        download.articles  = tests[i].articles;
        download.groups    = tests[i].groups;
        download.user_data = &params;

        ui::FileBatchDownload batch;
        batch.account = 123;
        batch.size    = 666;
        batch.path    = "test/foo/bar";
        batch.desc    = "download";
        batch.files.push_back(download);
        eng.DownloadFiles(batch);

        ui::TaskDesc task;
        eng.GetTask(0, &task);
        BOOST_REQUIRE(task.state == ui::TaskDesc::States::Active);

        ui::Connection conn;
        eng.GetConn(0, &conn);
        BOOST_REQUIRE(conn.state == ui::Connection::States::Active);
        BOOST_REQUIRE(conn.task == task.task_id);
        BOOST_REQUIRE(conn.account == 123);

        do
        {
            eng.RunMainThread();
            eng.Pump();
        }
        while (eng.HasPendingActions());

        eng.GetConn(0, &conn);
        BOOST_REQUIRE(conn.state == ui::Connection::States::Connected);
        BOOST_REQUIRE(conn.task == 0);
        BOOST_REQUIRE(conn.account == 123);

        eng.GetConn(1, &conn);
        BOOST_REQUIRE(conn.state == ui::Connection::States::Connected);
        BOOST_REQUIRE(conn.task == 0);
        BOOST_REQUIRE(conn.account == 321);

        eng.GetTask(0, &task);
        BOOST_REQUIRE(task.state == ui::TaskDesc::States::Complete);
        BOOST_REQUIRE(task.error == tests[i].errors);

        eng.KillConnection(0);
        eng.KillConnection(0);
        eng.Stop();
        do
        {
            eng.RunMainThread();
            eng.Pump();
        }
        while (eng.HasPendingActions());
    }
}

// Synopsis:
// Test that when content retrieval from fill server fails the task progresses
// as if the content was not available.
//
// Expected outcome:
// The task progresses, buffers are set to unavailable state.
//
// Issue: #3
void test_task_execute_fill_error()
{
    const bool debug_single_thread = true;

    Engine eng(std::make_unique<Factory>(), debug_single_thread);

    ConnState test_main_params;
    ConnState test_fill_params;
    test_main_params.force_no_such_body = true;
    test_fill_params.SetError(Connection::State::Active, Connection::Error::PermissionDenied);

    ui::Account main_account;
    main_account.id = 123;
    main_account.name = "main";
    main_account.username = "user";
    main_account.password = "pass";
    main_account.secure_host = "test.main.com";
    main_account.secure_port = 1000;
    main_account.connections = 1;
    main_account.enable_secure_server = true;
    main_account.enable_general_server = false;
    main_account.enable_compression = false;
    main_account.enable_pipelining = false;
    main_account.user_data = &test_main_params;

    ui::Account fill_account;
    fill_account.id = 321;
    fill_account.name = "fill";
    fill_account.username = "user";
    fill_account.password = "pass";
    fill_account.secure_host = "test.fill.com";
    fill_account.secure_port = 1000;
    fill_account.connections = 1;
    fill_account.enable_secure_server = true;
    fill_account.enable_general_server = false;
    fill_account.enable_compression = false;
    fill_account.enable_pipelining = false;
    fill_account.user_data = &test_fill_params;

    eng.SetAccount(main_account, false);
    eng.SetAccount(fill_account, false);
    eng.SetFillAccount(321);


    TaskParams params;
    params.should_commit = true;
    params.expect_cmdlist = true;
    params.num_buffers = 2;

    ui::FileDownload download;
    download.account   = 123;
    download.size      = 666;
    download.path      = "test/foo/bar";
    download.desc      = "download";
    download.articles.push_back("<failure>");
    download.articles.push_back("<failure>");
    download.groups.push_back("alt.binaries.success");
    download.user_data = &params;

    ui::FileBatchDownload batch;
    batch.account = 123;
    batch.size    = 666;
    batch.path    = "test/foo/bar";
    batch.desc    = "download";
    batch.files.push_back(download);
    eng.DownloadFiles(batch);

    eng.Start();

    while (eng.HasPendingActions())
    {
        eng.RunMainThread();
        eng.Pump();
    }

    ui::TaskDesc task;
    eng.GetTask(0, &task);

    BOOST_REQUIRE(task.state == ui::TaskDesc::States::Complete);
    BOOST_REQUIRE(task.error.test(ui::TaskDesc::Errors::Unavailable));
    BOOST_REQUIRE(task.error.test(ui::TaskDesc::Errors::Incomplete));

    eng.Stop();
    while (eng.HasPendingActions())
    {
        eng.RunMainThread();
        eng.Pump();
    }
}

void test_save_load_tasks()
{
    // empty task list, empty state
    {

        Engine eng(std::make_unique<Factory>(), true);
        eng.SaveTasks("tasks.bin");
        eng.LoadTasks("tasks.bin");
        BOOST_REQUIRE(eng.GetNumTasks() == 0);
    }

    // some tasks but no running yet.
    {

        const auto NumTestFiles = 300;

        Engine eng(std::make_unique<Factory>(), true);

        std::vector<TaskParams> params;
        params.resize(NumTestFiles);

        ui::FileBatchDownload batch;
        batch.account = 123;
        batch.size    = NumTestFiles * 100;
        batch.path    = "test/foo/bar";
        batch.desc    = "batch 1";
        for (int i=0; i<NumTestFiles; ++i)
        {
            params[i].num_buffers = 2;

            ui::FileDownload download;
            download.account = 123;
            download.size    = 100;
            download.path    = "test/foo/bar";
            download.desc    = "file " + std::to_string(i+1);
            download.articles.push_back("1");
            download.articles.push_back("2");
            download.groups.push_back("alt.binaries.foo");
            download.user_data = &params[i];
            batch.files.push_back(download);
        }
        eng.DownloadFiles(batch);

        std::deque<ui::TaskDesc> tasks;
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == NumTestFiles);
        for (int i=0; i<NumTestFiles; ++i)
        {
            BOOST_REQUIRE(tasks[i].desc == "file " + std::to_string(i+1));
            BOOST_REQUIRE(tasks[i].size == 100);
            BOOST_REQUIRE(tasks[i].account == 123);
            BOOST_REQUIRE(tasks[i].path == "test/foo/bar");
        }

        eng.SaveTasks("tasks.bin");
        eng.Reset();
        eng.LoadTasks("tasks.bin");

        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == NumTestFiles);
        for (int i=0; i<NumTestFiles; ++i)
        {
            BOOST_REQUIRE(tasks[i].desc == "file " + std::to_string(i+1));
            BOOST_REQUIRE(tasks[i].size == 100);
            BOOST_REQUIRE(tasks[i].account == 123);
            BOOST_REQUIRE(tasks[i].path == "test/foo/bar");
        }

        eng.SetGroupItems(true);
        eng.GetTasks(&tasks);
        BOOST_REQUIRE(tasks.size() == 1);
        BOOST_REQUIRE(tasks[0].account == 123);
        BOOST_REQUIRE(tasks[0].size == NumTestFiles * 100);
        BOOST_REQUIRE(tasks[0].path == "test/foo/bar");
        BOOST_REQUIRE(tasks[0].desc == "batch 1");
    }
}


int test_main(int argc, char*[])
{
    newsflash::initialize();
    newsflash::EnableDebugLog(true);

    test_connection_establish_success();
    test_connection_establish_failure();
    test_connection_reconnect();
    test_task_entry_and_delete();
    test_task_move();
    test_task_execute_success();
    test_task_execute_failure();
    test_task_execute_restart();
    test_task_execute_fill_success();
    test_task_execute_fill_error();

    test_save_load_tasks();
    return 0;
}
