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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <boost/random/mersenne_twister.hpp>
#include <newsflash/warnpop.h>
#include <thread>
#include <mutex>
#include <fstream>
#include <cassert>
#include <atomic>
#include <map>
#include "connection.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "buffer.h"
#include "session.h"
#include "cmdlist.h"
#include "action.h"
#include "event.h"
#include "socketapi.h"
#include "throttle.h"

namespace {

    // todo: refactor away this exception.
    class exception : public std::exception
    {
    public:
        exception(newsflash::connection::error err, const std::string& what)
            : error_(err), what_(std::move(what))
        {}

        const char* what() const NOTHROW // noexcept
        { return what_.c_str(); }

        newsflash::connection::error error() const NOTHROW // noexcept
        { return error_; }

    private:
        newsflash::connection::error error_;
        std::string what_;
    };
}

namespace newsflash
{

std::map<std::string, std::uint32_t> g_addr_cache;
std::mutex g_addr_cache_mutex;

struct connection::impl {
    std::mutex  mutex;
    std::string username;
    std::string password;
    std::string hostname;
    std::uint16_t port = 0;
    std::uint32_t addr = 0;
    std::atomic<std::uint64_t> bytes;
    std::unique_ptr<class socket> socket;
    std::unique_ptr<class session> session;
    std::unique_ptr<event> cancel;
    bool ssl = true;
    bool pipelining = false;
    bool compression = false;
    bool authenticate_immediately = false;
    double bps = 0.0;
    throttle* pthrottle = nullptr;
    boost::random::mt19937 random; // std::rand is not MT safe.

    connection::state state = state::disconnected;
    connection::error error = error::none;

    on_cmdlist_done on_cmdlist_done_callback;

    void do_auth(std::string& user, std::string& pass) const
    {
        if (username.empty() || password.empty())
            throw exception(error::no_permission, "authentication requried but no credentials provided");
        user = username;
        pass = password;
    }
    void do_send(const std::string& cmd)
    {
        socket->sendall(&cmd[0], cmd.size());
    }
};

// perform host resolution
class connection::resolve : public action
{
public:
    resolve(std::shared_ptr<impl> s) : state_(s)
    {}

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        const auto hostname = state_->hostname;

        LOG_D("Resolving hostname ", hostname);

        std::uint32_t addr = 0;

        {
            std::lock_guard<std::mutex> lock(g_addr_cache_mutex);
            auto it = g_addr_cache.find(hostname);
            if (it != std::end(g_addr_cache))
                addr = it->second;
        }

        if (addr)
        {
            LOG_D("Using cached address");
        }
        else
        {
            const auto err = resolve_host_ipv4(hostname, addr);
            if (err)
            {
                LOG_E("Failed to resolve host (", err.value(), ") ", err.message());
                throw exception(error::resolve, "resolve failed");
            }

            {
                std::lock_guard<std::mutex> lock(g_addr_cache_mutex);
                g_addr_cache.insert(std::make_pair(hostname, addr));
            }
        }
        state_->addr = addr;

        LOG_I("Hostname resolved to ", ipv4{state_->addr});
    }

    virtual std::string describe() const override
    {
        return "Resolve " + state_->hostname;
    }
private:
    std::shared_ptr<impl> state_;
};

// perform socket connect
class connection::connect : public action
{
public:
    connect(std::shared_ptr<impl> s) : state_(s)
    {
        if (state_->ssl)
            state_->socket.reset(new sslsocket);
        else state_->socket.reset(new tcpsocket);
    }

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        LOG_I("Connecting to ", ipv4{state_->addr}, ", ", state_->port);

        const auto& socket = state_->socket;
        const auto& cancel = state_->cancel;
        const auto addr = state_->addr;
        const auto port = state_->port;

        // first begin connection (async)
        socket->begin_connect(addr, port);

        // wait for completion or cancellation event.
        // when an async socket connect is performed the socket will become writeable
        // once the connection is established.
        auto connected = socket->wait(false, true);
        auto canceled  = cancel->wait();
        if (!newsflash::wait_for(connected, canceled, std::chrono::seconds(10)))
            throw exception(error::timeout, "connection attempt timed out");
        if (canceled)
        {
            LOG_D("Connection was canceled");
            return;
        }

        socket->complete_connect();

        LOG_I("Socket connection ready!");
    }

    virtual std::string describe() const override
    {
        return str("Connect to ", ipv4{state_->addr}, ":", state_->port);
    }
private:
    std::shared_ptr<impl> state_;
};

// perform nntp init
class connection::initialize : public action
{
public:
    initialize(std::shared_ptr<impl> s) : state_(s)
    {
        state_->session.reset(new session);

        // there was a lambda here before but the lambda took the state
        // object by a shared_ptr and created a circular depedency.
        // i think a cleaner and less error prone system is
        // to use a boost:bind here with real functions.

        state_->session->on_auth = std::bind(&impl::do_auth, state_.get(),
            std::placeholders::_1, std::placeholders::_2);

        state_->session->on_send = std::bind(&impl::do_send, state_.get(),
            std::placeholders::_1);

        state_->session->enable_pipelining(s->pipelining);
        state_->session->enable_compression(s->compression);
    }
    virtual std::string describe() const override
    {
        return "Initialize NNTP session";
    }

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        LOG_I("Initializing NNTP session");
        LOG_I("Enable gzip compress: ", state_->compression);
        LOG_I("Enable pipelining: ", state_->pipelining);

        auto& session = state_->session;
        auto& socket  = state_->socket;
        auto& cancel  = state_->cancel;

        // begin new session
        session->start(state_->authenticate_immediately);

        newsflash::buffer buff(1024);
        newsflash::buffer temp;

        // while there are pending commands in the session we read
        // data from the socket into the buffer and then feed the buffer
        // into the session to update the session state.
        while (session->send_next())
        {
            do
            {
                // wait for data or cancellation
                auto received = socket->wait(true, false);
                auto canceled = cancel->wait();
                if (!newsflash::wait_for(received, canceled, std::chrono::seconds(5)))
                    throw exception(connection::error::timeout, "connection timeout");
                if (canceled)
                {
                    LOG_D("Initialize was canceled");
                    return;
                }

                // readsome data
                const auto bytes = socket->recvsome(buff.back(), buff.available());
                if (bytes == 0)
                    throw exception(connection::error::network, "socket was closed unexpectedly");

                // commit
                buff.append(bytes);
            }
            while (!session->recv_next(buff, temp));
        }

        // check for errors
        const auto err = session->get_error();

        if (err == session::error::authentication_rejected)
            throw exception(connection::error::authentication_rejected, "authentication rejected");
        else if (err == session::error::no_permission)
            throw exception(connection::error::no_permission, "no permission");
        else if (err == session::error::protocol)
            throw exception(connection::error::protocol, "protocol error");

        LOG_I("NNTP Session ready");

    }
private:
    std::shared_ptr<impl> state_;
};

// execute cmdlist
class connection::execute : public action
{
public:
    execute(std::shared_ptr<impl> s, std::shared_ptr<CmdList> cmd, std::size_t tid) : state_(s), cmds_(cmd), taskid_(tid)
    {}

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        auto& session  = state_->session;
        auto& cancel   = state_->cancel;
        auto& socket   = state_->socket;
        auto* throttle = state_->pthrottle;
        auto& cmdlist  = cmds_;

        LOG_D("Execute cmdlist ", cmdlist->GetCmdListId());
        LOG_D("Cmdlist has ", cmdlist->NumDataCommands(), " data commands");

        newsflash::buffer recvbuf(MB(4));

        // the cmdlist contains a list of commands
        // we pass the session object to the cmdlist to allow
        // it to submit a request into the session.
        // then while there are pending commands in the session
        // we read data from the socket into the buffer and pass
        // it into the session in order to update the session state.
        // once a command is completed we pass the output buffer
        // to the cmdlist so that it can update its own state.
        if (cmdlist->IsCancelled())
        {
            LOG_D("Cmdlist was canceled");
            return;
        }

        if (cmdlist->NeedsToConfigure())
        {
            bool configure_success = false;

            for (std::size_t i=0; ; ++i)
            {
                if (!cmdlist->SubmitConfigureCommand(i, *session))
                    break;

                if (!session->pending()) {
                    configure_success = true;
                    break;
                }

                newsflash::buffer config(KB(1));

                while (session->send_next())
                {
                    do
                    {
                        auto received = socket->wait(true, false);
                        auto canceled = cancel->wait();
                        if (!newsflash::wait_for(received, canceled, std::chrono::seconds(10)))
                            throw exception(connection::error::timeout, "connection timeout");
                        else if (canceled)
                            return;

                        const auto bytes = socket->recvsome(recvbuf.back(), recvbuf.available());
                        if (bytes == 0)
                            throw exception(connection::error::network, "connection was closed unexpectedly");

                        recvbuf.append(bytes);
                    }
                    while (!session->recv_next(recvbuf, config));
                }

                const auto err = session->get_error();
                if (err == session::error::authentication_rejected)
                    throw exception(connection::error::authentication_rejected, "authentication rejected");
                else if (err == session::error::no_permission)
                    throw exception(connection::error::no_permission, "no permission");
                else if (err == session::error::protocol)
                    throw exception(connection::error::protocol, "protocol error");

                if (cmdlist->ReceiveConfigureBuffer(i, std::move(config)))
                {
                    configure_success = true;
                    break;
                }
            }
            if (!configure_success)
            {
                LOG_E("Cmdlist session configuration failed");
                return;
            }
        } // needs to configure

        if (cmdlist->IsCancelled())
        {
            LOG_D("Cmdlist was canceled");
            return;
        }

        LOG_I("Submit data commands");

        cmdlist->SubmitDataCommands(*session);

        LOG_FLUSH();

        state_->bps = 0;

        using clock = std::chrono::steady_clock;

        const auto start = clock::now();

        std::uint64_t accum = 0;

        while (session->pending())
        {
            newsflash::buffer content(MB(4));

            session->send_next();
            do
            {
                auto canceled = cancel->wait();
                if (newsflash::wait_for(canceled, std::chrono::milliseconds(0)))
                    return;

                if (!socket->can_recv())
                {
                    // wait for data or cancellation
                    auto received = socket->wait(true, false);
                    auto canceled = cancel->wait();
                    if (!newsflash::wait_for(received, canceled, std::chrono::seconds(30)))
                        throw exception(connection::error::timeout, "connection timeout");
                    else if (canceled)
                        return;
                }

                auto quota = throttle->give_quota();
                while (!quota)
                {
                    const auto ms = state_->random() % 50;
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                    quota = throttle->give_quota();
                }

                std::size_t avail = std::min(recvbuf.available(), quota);

                // readsome
                const auto bytes = socket->recvsome(recvbuf.back(), avail);
                if (bytes == 0)
                    throw exception(connection::error::network, "connection was closed unexpectedly");

                //LOG_D("Quota ", quota, " avail ", recvbuf.available(), " recvd ", bytes);

                throttle->accumulate(bytes, quota);

                recvbuf.append(bytes);
                accum += bytes;

                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start);
                const auto seconds = ms.count() / 1000.0;
                const auto bps = accum / seconds;
                state_->bps    = 0.05 * bps + (0.95 * state_->bps);
                state_->bytes += bytes;
                total_bytes_ += bytes;
            }
            while (!session->recv_next(recvbuf, content));

            content_bytes_ += content.content_length();

            // todo: is this oK? (in case when quota finishes..??)
            const auto err = session->get_error();
            if (err != session::error::none)
                throw exception(connection::error::no_permission, "no permission");

            cmdlist->ReceiveDataBuffer(std::move(content));

            // if the session is pipelined there's no way to stop the data transmission
            // of already pipelined commands other than by doing a hard socket reset.
            // if the session is not pipelined we can just exit the reading loop after
            // a complete command is completed and we can maintain the socket/session.
            if (cmdlist->IsCancelled())
            {
                LOG_D("Cmdlist was canceled");

                if (state_->pipelining)
                    throw exception(connection::error::pipeline_reset, "pipeline reset");

                session->clear();
                return;
            }
        }

        LOG_I("Cmdlist complete");
    }

    virtual void run_completion_callbacks() override
    {
        connection::cmdlist_completion_data completion;
        completion.cmds          = cmds_;
        completion.task_owner_id = taskid_;
        completion.total_bytes   = total_bytes_;
        completion.content_bytes = content_bytes_;
        completion.success       = has_exception() == false;
        state_->on_cmdlist_done_callback(completion);
    }

    virtual std::string describe() const override
    {
        return "Execute cmdlist";
    }
private:
    std::shared_ptr<impl> state_;
    std::shared_ptr<CmdList> cmds_;
    std::size_t taskid_ = 0;
    std::size_t total_bytes_ = 0;
    std::size_t content_bytes_ = 0;
private:
};

class connection::disconnect : public action
{
public:
    disconnect(std::shared_ptr<impl> s) : state_(s)
    {}

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        LOG_I("Disconnect");

        auto& session = state_->session;
        auto& socket  = state_->socket;

        // if the connection is disconnecting while there are pending
        // transactions in the session we're just simply going to close the socket
        // and not perform a clean protocol shutdown.
        if (!session->pending())
        {
            newsflash::buffer buff(64);
            newsflash::buffer temp;

            session->quit();
            session->send_next();
            do
            {
            // wait for data, if no response then khtx bye whatever, we're done anyway
                auto received = socket->wait(true, false);
                if (!newsflash::wait_for(received, std::chrono::seconds(1)))
                    break;
                if (buff.available() == 0)
                    buff.allocate(buff.size() + 64);

                const auto bytes = socket->recvsome(buff.back(), buff.available());
                if (bytes == 0)
                {
                    LOG_D("Received socket close");
                    break;
                }
                buff.append(bytes);
            }
            while (!session->recv_next(buff, temp));
        }

        socket->close();

        LOG_D("Disconnect complete");
    }

    virtual std::string describe() const override
    {
        return "Disconnect";
    }
private:
    std::shared_ptr<impl> state_;
};

class connection::ping : public action
{
public:
    ping(std::shared_ptr<impl> s) : state_(s)
    {}

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        LOG_D("Perform ping");

        auto& session = state_->session;
        auto& socket  = state_->socket;

        newsflash::buffer buff(64);
        newsflash::buffer temp;

        session->ping();
        while (session->pending())
        {
            session->send_next();
            do
            {
                auto received = socket->wait(true, false);
                if (!newsflash::wait_for(received, std::chrono::seconds(4)))
                    throw exception(connection::error::timeout, "connection timeout (no ping)");

                const auto bytes = socket->recvsome(buff.back(), buff.available());
                if (bytes == 0)
                    throw exception(connection::error::network, "connection was closed unexpectedly");

                buff.append(bytes);
            }
            while (!session->recv_next(buff, temp));
        }
    }

    virtual std::string describe() const override
    {
        return "Ping";
    }
private:
    std::shared_ptr<impl> state_;
};

connection::connection()
{
    state_ = std::make_shared<impl>();
}

std::unique_ptr<action> connection::connect(const spec& s)
{
    state_->username = s.username;
    state_->password = s.password;
    state_->hostname = s.hostname;
    state_->port     = s.hostport;
    state_->addr     = 0;
    state_->bytes    = 0;
    state_->ssl      = s.use_ssl;
    state_->compression = s.enable_compression;
    state_->pipelining = s.enable_pipelining;
    state_->authenticate_immediately = s.authenticate_immediately;
    state_->bps = 0;
    state_->socket.reset();
    state_->cancel.reset(new event);
    state_->cancel->reset();
    state_->pthrottle = s.pthrottle;
    state_->random.seed((std::size_t)this);
    state_->state = state::resolving;
    state_->error = error::none;
    std::unique_ptr<action> act(new resolve(state_));

    return act;
}

std::unique_ptr<action> connection::disconnect()
{
    std::unique_ptr<action> a(new class disconnect(state_));

    return a;
}

std::unique_ptr<action> connection::ping()
{
    std::unique_ptr<action> a(new class ping(state_));

    return a;
}

std::unique_ptr<action> connection::complete(std::unique_ptr<action> a)
{
    std::unique_ptr<action> next;
    try
    {
        auto* ptr = a.get();

        // rethrow an exception if any
        // todo: we should refactor the socket api to use error codes instead of exceptions,
        // since these are logical errors and leave the actual exceptions for real errors.
        ptr->rethrow();

        if (dynamic_cast<resolve*>(ptr))
        {
            next.reset(new class connect(state_));
            state_->state = state::connecting;
        }
        else if (dynamic_cast<class connect*>(ptr))
        {
            next.reset(new class initialize(state_));
            state_->state = state::initializing;
        }
        else if (dynamic_cast<class initialize*>(ptr))
        {
            state_->state = state::connected;
        }
        else if (dynamic_cast<class disconnect*>(ptr))
        {
            state_->state = state::disconnected;
        }
        else if (auto* p = dynamic_cast<class execute*>(ptr))
        {
            state_->state = state::connected;
        }
    }
    catch (const exception& e)
    {
        state_->state = state::error;
        state_->error = e.error();
    }
    catch (const std::system_error& e)
    {
        const auto code = e.code();
        if (code == std::errc::connection_refused)
        {
            state_->state = state::error;
            state_->error = error::refused;
        }
        else if (code == std::errc::connection_reset)
        {
            state_->state = state::error;
            state_->error = error::reset;
        }
        else
        {
            throw e;
        }
    }
    return next;
}

std::unique_ptr<action> connection::execute(std::shared_ptr<CmdList> cmd, std::size_t tid)
{
    state_->cancel->reset();

    std::unique_ptr<action> act(new class execute(state_, std::move(cmd), tid));

    state_->state = state::active;

    return act;
}

void connection::cancel()
{
    state_->cancel->set();
}

std::string connection::username() const
{ return state_->username; }

std::string connection::password() const
{ return state_->password; }

std::uint32_t connection::current_speed_bps() const
{ return (std::uint32_t)state_->bps; }

std::uint64_t connection::num_bytes_transferred() const
{ return (std::uint64_t)state_->bytes; }

connection::state connection::get_state() const
{ return state_->state; }

connection::error connection::get_error() const
{ return state_->error; }

void connection::set_callback(const on_cmdlist_done& callback)
{
    state_->on_cmdlist_done_callback = callback;
}

} // newsflash
