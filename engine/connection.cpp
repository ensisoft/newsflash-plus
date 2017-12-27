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
#  include <boost/random/mersenne_twister.hpp>
#include "newsflash/warnpop.h"

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

namespace newsflash
{

std::map<std::string, std::uint32_t> g_addr_cache;
std::mutex g_addr_cache_mutex;

struct ConnectionImpl::impl {
    std::mutex  mutex;
    std::string username;
    std::string password;
    std::string hostname;
    std::uint16_t port = 0;
    std::uint32_t addr = 0;
    std::atomic<std::uint64_t> bytes;
    std::unique_ptr<Socket> socket;
    std::unique_ptr<Session> session;
    std::unique_ptr<Event> cancel;
    bool ssl = true;
    bool pipelining = false;
    bool compression = false;
    bool authenticate_immediately = false;
    double bps = 0.0;
    throttle* pthrottle = nullptr;
    boost::random::mt19937 random; // std::rand is not MT safe.

    std::error_code pending_socket_error;
    Session::Error  pending_session_error = Session::Error::None;
    Connection::Error pending_connection_error = Connection::Error::None;

    ConnectionImpl::State state = State::Disconnected;
    ConnectionImpl::Error error = Error::None;

    OnCmdlistDone on_cmdlist_done_callback;

    void do_auth(std::string& user, std::string& pass) const
    {
        user = username;
        pass = password;
    }
    void do_send(const std::string& cmd)
    {
        std::error_code error;
        socket->SendAll(&cmd[0], cmd.size(), &error);
        if (error)
        {
            pending_socket_error = error;
        }
    }
};

// perform host resolution
class ConnectionImpl::resolve : public action
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
                state_->pending_connection_error = Error::Resolve;
                return;
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
class ConnectionImpl::connect : public action
{
public:
    connect(std::shared_ptr<impl> s) : state_(s)
    {
        if (state_->ssl)
            state_->socket.reset(new SslSocket);
        else state_->socket.reset(new TcpSocket);
    }

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        LOG_I("Connecting to ", ipv4{state_->addr}, ", ", state_->port);

        const auto& socket = state_->socket;
        const auto& cancel = state_->cancel;
        const auto addr = state_->addr;
        const auto port = state_->port;

        // first begin ConnectionImpl (async)
        socket->BeginConnect(addr, port);

        // wait for completion or cancellation event.
        // when an async socket connect is performed the socket will become writeable
        // once the ConnectionImpl is established.
        auto connected = socket->GetWaitHandle(false, true);
        auto cancelled = cancel->GetWaitHandle();
        if (!newsflash::WaitForMultipleHandles(connected, cancelled, std::chrono::seconds(10)))
        {
            state_->pending_connection_error = Error::Timeout;
            return;
        }
        if (cancelled)
        {
            LOG_D("Connection was canceled");
            return;
        }

        std::error_code connection_error;
        socket->CompleteConnect(&connection_error);
        if (connection_error)
        {
            state_->pending_socket_error = connection_error;
            return;
        }

        LOG_I("Socket connection ready");
    }

    virtual std::string describe() const override
    {
        return str("Connect to ", ipv4{state_->addr}, ":", state_->port);
    }
private:
    std::shared_ptr<impl> state_;
};

// perform nntp init
class ConnectionImpl::initialize : public action
{
public:
    initialize(std::shared_ptr<impl> s) : state_(s)
    {
        state_->session.reset(new Session);

        // there was a lambda here before but the lambda took the state
        // object by a shared_ptr and created a circular depedency.
        // i think a cleaner and less error prone system is
        // to use a boost:bind here with real functions.

        state_->session->on_auth = std::bind(&impl::do_auth, state_.get(),
            std::placeholders::_1, std::placeholders::_2);

        state_->session->on_send = std::bind(&impl::do_send, state_.get(),
            std::placeholders::_1);

        state_->session->SetEnablePipelining(s->pipelining);
        state_->session->SetEnableCompression(s->compression);
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
        session->Start(state_->authenticate_immediately);

        Buffer buff(1024);
        Buffer temp;

        // while there are pending commands in the session we read
        // data from the socket into the buffer and then feed the buffer
        // into the session to update the session state.
        while (session->SendNext())
        {
            do
            {
                // wait for data or cancellation
                auto received  = socket->GetWaitHandle(true, false);
                auto cancelled = cancel->GetWaitHandle();
                if (!newsflash::WaitForMultipleHandles(received, cancelled, std::chrono::seconds(5)))
                {
                    state_->pending_connection_error = Error::Timeout;
                    return;
                }

                if (cancelled)
                {
                    LOG_D("Initialize was cancelled");
                    return;
                }

                // readsome data
                std::error_code recv_error;
                const auto bytes = socket->RecvSome(buff.Back(), buff.GetAvailableBytes(), &recv_error);
                if (recv_error)
                {
                    state_->pending_socket_error = recv_error;
                    return;
                }
                else if (bytes == 0)
                {
                    state_->pending_connection_error = Error::Network;
                    return;
                }

                // commit
                buff.Append(bytes);
            }
            while (!session->RecvNext(buff, temp));
        }

        // check for errors
        const auto err = session->GetError();
        if (err != Session::Error::None)
        {
            state_->pending_session_error = err;
            return;
        }
        LOG_I("NNTP Session ready");
    }
private:
    std::shared_ptr<impl> state_;
};

// execute cmdlist
class ConnectionImpl::execute : public action
{
public:
    execute(std::shared_ptr<impl> s, std::shared_ptr<CmdList> cmd) : state_(s), cmds_(cmd)
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

        Buffer recvbuf(MB(4));

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

                if (!session->HasPending()) {
                    configure_success = true;
                    break;
                }

                Buffer config(KB(1));

                while (session->SendNext())
                {
                    do
                    {
                        auto received  = socket->GetWaitHandle(true, false);
                        auto cancelled = cancel->GetWaitHandle();
                        if (!newsflash::WaitForMultipleHandles(received, cancelled, std::chrono::seconds(10)))
                        {
                            state_->pending_connection_error = Error::Timeout;
                            return;
                        }
                        else if (cancelled)
                            return;

                        std::error_code recv_error;
                        const auto bytes = socket->RecvSome(recvbuf.Back(), recvbuf.GetAvailableBytes(), &recv_error);
                        if (recv_error)
                        {
                            state_->pending_socket_error = recv_error;
                            return;
                        }
                        else if (bytes == 0)
                        {
                            state_->pending_connection_error = Error::Network;
                            return;
                        }
                        recvbuf.Append(bytes);
                    }
                    while (!session->RecvNext(recvbuf, config));
                }

                const auto err = session->GetError();
                if (err != Session::Error::None)
                {
                    state_->pending_session_error = err;
                    return;
                }

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

        while (session->HasPending())
        {
            Buffer content(MB(4));

            session->SendNext();
            do
            {
                auto cancelled = cancel->GetWaitHandle();
                if (newsflash::WaitForSingleHandle(cancelled, std::chrono::milliseconds(0)))
                    return;

                if (!socket->CanRecv())
                {
                    // wait for data or cancellation
                    auto received  = socket->GetWaitHandle(true, false);
                    auto cancelled = cancel->GetWaitHandle();
                    if (!newsflash::WaitForMultipleHandles(received, cancelled, std::chrono::seconds(30)))
                    {
                        state_->pending_connection_error = Error::Timeout;
                        return;
                    }
                    else if (cancelled)
                        return;
                }

                auto quota = throttle->give_quota();
                while (!quota)
                {
                    const auto ms = state_->random() % 50;
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                    quota = throttle->give_quota();
                }

                std::size_t avail = std::min(recvbuf.GetAvailableBytes(), quota);

                // readsome
                std::error_code recv_error;
                const auto bytes = socket->RecvSome(recvbuf.Back(), avail, &recv_error);
                if (recv_error)
                {
                    state_->pending_socket_error = recv_error;
                    return;
                }
                else if (bytes == 0)
                {
                    state_->pending_connection_error = Error::Network;
                    return;
                }

                //LOG_D("Quota ", quota, " avail ", recvbuf.available(), " recvd ", bytes);

                throttle->accumulate(bytes, quota);

                recvbuf.Append(bytes);
                accum += bytes;

                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start);
                const auto seconds = ms.count() / 1000.0;
                const auto bps = accum / seconds;
                state_->bps    = 0.05 * bps + (0.95 * state_->bps);
                state_->bytes += bytes;
                total_bytes_ += bytes;
            }
            while (!session->RecvNext(recvbuf, content));

            content_bytes_ += content.GetContentLength();

            // todo: is this oK? (in case when quota finishes..??)
            const auto err = session->GetError();
            if (err != Session::Error::None)
            {
                state_->pending_session_error = err;
                return;
            }

            cmdlist->ReceiveDataBuffer(std::move(content));

            // if the session is pipelined there's no way to stop the data transmission
            // of already pipelined commands other than by doing a hard socket reset.
            // if the session is not pipelined we can just exit the reading loop after
            // a complete command is completed and we can maintain the socket/session.
            if (cmdlist->IsCancelled())
            {
                LOG_D("Cmdlist was canceled");

                if (state_->pipelining)
                {
                    state_->pending_connection_error = Error::PipelineReset;
                    return;
                }
                session->Clear();
                return;
            }
        }

        LOG_I("Cmdlist complete");
    }

    virtual void run_completion_callbacks() override
    {
        const bool has_exception = this->has_exception();
        const bool has_connection_error = state_->pending_connection_error != Connection::Error::None;
        const bool has_socket_error = state_->pending_socket_error != std::error_code();
        const bool has_session_error = state_->pending_session_error != Session::Error::None;
        const bool has_any_error =
            has_exception || has_connection_error || has_socket_error ||
            has_session_error;

        Connection::CmdListCompletionData completion;
        completion.cmds          = cmds_;
        completion.total_bytes   = total_bytes_;
        completion.content_bytes = content_bytes_;
        completion.execution_did_complete = !has_any_error;
        state_->on_cmdlist_done_callback(completion);
    }

    virtual std::string describe() const override
    {
        return "Execute cmdlist";
    }
private:
    std::shared_ptr<impl> state_;
    std::shared_ptr<CmdList> cmds_;
    std::size_t total_bytes_ = 0;
    std::size_t content_bytes_ = 0;
private:
};

class ConnectionImpl::disconnect : public action
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

        // if the ConnectionImpl is disconnecting while there are pending
        // transactions in the session we're just simply going to close the socket
        // and not perform a clean protocol shutdown.
        if (!session->HasPending())
        {
            Buffer buff(64);
            Buffer temp;

            session->Quit();
            session->SendNext();
            do
            {
                // wait for data, if no response then khtx bye whatever, we're done anyway
                auto received = socket->GetWaitHandle(true, false);
                if (!newsflash::WaitForSingleHandle(received, std::chrono::seconds(1)))
                    break;
                if (buff.IsFull())
                    buff.Grow(+64);

                std::error_code recv_error;
                const auto bytes = socket->RecvSome(buff.Back(), buff.GetAvailableBytes(), &recv_error);
                if (recv_error)
                {
                    state_->pending_socket_error = recv_error;
                    return;
                }
                else if (bytes == 0)
                {
                    LOG_D("Received socket close");
                    break;
                }
                buff.Append(bytes);
            }
            while (!session->RecvNext(buff, temp));
        }

        socket->Close();

        LOG_D("Disconnect complete");
    }

    virtual std::string describe() const override
    {
        return "Disconnect";
    }
private:
    std::shared_ptr<impl> state_;
};

class ConnectionImpl::ping : public action
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

        Buffer buff(64);
        Buffer temp;

        session->Ping();
        while (session->HasPending())
        {
            session->SendNext();
            do
            {
                auto received = socket->GetWaitHandle(true, false);
                if (!newsflash::WaitForSingleHandle(received, std::chrono::seconds(4)))
                {
                    state_->pending_connection_error = Error::Timeout;
                    return;
                }
                std::error_code recv_error;
                const auto bytes = socket->RecvSome(buff.Back(), buff.GetAvailableBytes(), &recv_error);
                if (recv_error)
                {
                    state_->pending_socket_error = recv_error;
                    return;
                }
                else if (bytes == 0)
                {
                    state_->pending_connection_error = Error::Network;
                    return;
                }
                buff.Append(bytes);
            }
            while (!session->RecvNext(buff, temp));
        }
    }

    virtual std::string describe() const override
    {
        return "Ping";
    }
private:
    std::shared_ptr<impl> state_;
};

ConnectionImpl::ConnectionImpl()
{
    state_ = std::make_shared<impl>();
}

std::unique_ptr<action> ConnectionImpl::Connect(const HostDetails& s)
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
    state_->cancel.reset(new Event);
    state_->cancel->ResetSignal();
    state_->pthrottle = s.pthrottle;
    state_->random.seed((std::size_t)this);
    state_->state = State::Resolving;
    state_->error = Error::None;
    state_->pending_socket_error = std::error_code();
    state_->pending_session_error = Session::Error::None;
    state_->pending_connection_error = Connection::Error::None;
    std::unique_ptr<action> act(new resolve(state_));

    return act;
}

std::unique_ptr<action> ConnectionImpl::Disconnect()
{
    std::unique_ptr<action> a(new class disconnect(state_));

    return a;
}

std::unique_ptr<action> ConnectionImpl::Ping()
{
    std::unique_ptr<action> a(new class ping(state_));

    return a;
}

std::unique_ptr<action> ConnectionImpl::Complete(std::unique_ptr<action> a)
{
    std::unique_ptr<action> next;

    // map different levels of errors to higher level connection error.
    if (state_->pending_socket_error)
    {
        state_->state = State::Error;
        if (state_->pending_socket_error == std::errc::connection_refused)
            state_->error = Error::Refused;
        else if (state_->pending_socket_error == std::errc::connection_reset)
            state_->error = Error::Reset;
        else state_->error = Error::Other;

        LOG_E("Socket error ", state_->pending_socket_error);
    }
    else if (state_->pending_session_error != Session::Error::None)
    {
        state_->state = State::Error;
        switch (state_->pending_session_error)
        {
            case Session::Error::None:
                break;
            case Session::Error::AuthenticationRejected:
                state_->error = Error::AuthenticationRejected;
                break;
            case Session::Error::NoPermission:
                state_->error = Error::PermissionDenied;
                break;
            case Session::Error::Protocol:
                state_->error = Error::Protocol;
                break;
        }
    }
    else if (state_->pending_connection_error != Connection::Error::None)
    {
        state_->state = State::Error;
        state_->error = state_->pending_connection_error;
    }

    if (state_->state == State::Error)
    {
        switch (state_->error)
        {
            case Error::None: break;
            case Error::Resolve:
                LOG_E("Failed to resolve host");
                break;
            case Error::Refused:
                LOG_E("Connection was refused");
                break;
            case Error::Reset:
                LOG_E("Connection was reset");
                break;
            case Error::Protocol:
                LOG_E("NNTP protocol error");
                break;
            case Error::AuthenticationRejected:
                LOG_E("Session authentication was rejected");
                break;
            case Error::PermissionDenied:
                LOG_E("Session permission denied");
                break;
            case Error::Timeout:
                LOG_E("Connection timed out");
                break;
            case Error::Network:
                LOG_E("Connection was closed unexpectedly");
                break;
            case Error::PipelineReset:
                LOG_E("Pipelined commands forced connection reset");
                break;
            case Error::Other:
                LOG_E("Connection error not known");
                break;
        }
        return next;
    }

    auto* ptr = a.get();

    if (dynamic_cast<resolve*>(ptr))
    {
        next.reset(new class connect(state_));
        state_->state = State::Connecting;
    }
    else if (dynamic_cast<class connect*>(ptr))
    {
        next.reset(new class initialize(state_));
        state_->state = State::Initializing;
    }
    else if (dynamic_cast<class initialize*>(ptr))
    {
        state_->state = State::Connected;
    }
    else if (dynamic_cast<class disconnect*>(ptr))
    {
        state_->state = State::Disconnected;
    }
    else if (auto* p = dynamic_cast<class execute*>(ptr))
    {
        state_->state = State::Connected;
    }
    return next;
}

std::unique_ptr<action> ConnectionImpl::Execute(std::shared_ptr<CmdList> cmd)
{
    state_->cancel->ResetSignal();

    std::unique_ptr<action> act(new class execute(state_, std::move(cmd)));

    state_->state = State::Active;

    return act;
}

void ConnectionImpl::Cancel()
{
    state_->cancel->SetSignal();
}

std::string ConnectionImpl::GetUsername() const
{ return state_->username; }

std::string ConnectionImpl::GetPassword() const
{ return state_->password; }

std::uint32_t ConnectionImpl::GetCurrentSpeedBps() const
{ return (std::uint32_t)state_->bps; }

std::uint64_t ConnectionImpl::GetNumBytesTransferred() const
{ return (std::uint64_t)state_->bytes; }

Connection::State ConnectionImpl::GetState() const
{ return state_->state; }

Connection::Error ConnectionImpl::GetError() const
{ return state_->error; }

void ConnectionImpl::SetCallback(const OnCmdlistDone& callback)
{
    state_->on_cmdlist_done_callback = callback;
}

} // newsflash
