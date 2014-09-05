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

#include <cassert>
#include "connection.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "logging.h"
#include "buffer.h"
#include "session.h"
#include "cmdlist.h"
#include "action.h"
#include "event.h"
#include "sockets.h"

namespace newsflash
{

class connection::exception : public std::exception
{
public:
    exception(std::string what, connection::error err) : what_(std::move(what)), error_(err)
    {}
    exception(std::string what, std::error_code errc, connection::error err) : what_(std::move(what)), code_(errc), error_(err)
    {}

    const char* what() const noexcept
    { return what_.c_str(); }

    connection::error error() const noexcept
    { return error_; }

    std::error_code code() const noexcept
    { return code_; }
private:
    std::string what_;
    std::error_code code_;
    connection::error error_;
};

// resolve the host name to an ipv4 address 
class connection::resolve : public action
{
public:
    resolve(std::string hostname) : hostname_(std::move(hostname)), address_(0)
    {}

    virtual void xperform() override
    {
#if defined(LINUX_OS)
        struct addrinfo* addrs = nullptr;
        const auto ret = getaddrinfo(hostname_.c_str(), nullptr, nullptr, &addrs);
        if (ret == EAI_SYSTEM)
            throw exception("resolve host error", std::error_code(errno, std::generic_category()), connection::error::resolve);
        else if (ret != 0)
            throw exception("resolve host error", connection::error::resolve);

        std::uint32_t host = 0;
        const struct addrinfo* iter = addrs;
        while (iter)
        {
            if (iter->ai_addrlen == sizeof(sockaddr_in))
            {
                const auto* ptr = static_cast<const sockaddr_in*>((void*)iter->ai_addr);
                host = ptr->sin_addr.s_addr;
                break;
            }
            iter = iter->ai_next;
        }
        freeaddrinfo(addrs);
        address_ = ntohl(host);

#elif defined(WINDOWS_OS)
        const auto* hostent = gethostbyname(hostname.c_str());
        if (hostent == nullptr)
        {
            throw exception("resolve host error", std::error_code(WSAGetLastError(), std::system_category()),
                connection::error::resolve);
        }
        const auto* addr = static_cast<const in_addr*>((void*)hostent->h_addr);
        address_ = ntohl(addr->s_addr);
#endif
    }

    ipv4addr_t get_resolved_address() const 
    { return address_; }

private:
    std::string hostname_;
private:
    std::uint32_t address_;
};

// try to connect a socket to the given resolved host address and port
class connection::connect : public action
{
public:
    connect(event& cancel, ipv4addr_t addr, ipv4port_t port, bool ssl) : addr_(addr), port_(port), cancel_(cancel)
    {
        if (ssl)
            socket_.reset(new sslsocket);
        else socket_.reset(new tcpsocket);
    }

    virtual void xperform() override
    {
        // first begin connection (async)
        socket_->begin_connect(addr_, port_);

        // wait for completion or cancellation event.
        auto connect = socket_->wait(true, false);
        auto cancel  = cancel_.wait();
        newsflash::wait(connect, cancel);
        if (cancel)
            return;

        // complete connect, throws on error
        socket_->complete_connect();
    }
    
    std::unique_ptr<socket> get_connected_socket() 
    { return std::move(socket_); }

private:
    std::unique_ptr<socket> socket_;
private:    
    ipv4addr_t addr_;
    ipv4port_t port_;
private: 
    event& cancel_;
};

// initialize the session object
class connection::initialize : public action
{
public:
    initialize(event& cancel, std::unique_ptr<socket> socket, std::string username, std::string password) 
       : socket_(std::move(socket)), username_(std::move(username)), password_(std::move(password)), cancel_(cancel)

    {
        session_.reset(new session);
        session_->on_auth = std::bind(&initialize::do_auth, this, 
            std::placeholders::_1, std::placeholders::_2);
        session_->on_send = std::bind(&initialize::do_send, this,
            std::placeholders::_1);
    }
    virtual void xperform() override
    {
        // begin new session
        session_->start();

        newsflash::buffer buff(1024);
        newsflash::buffer temp(1);

        // while there are pending commands in the session we read 
        // data from the socket into the buffer and then feed the buffer
        // into the session to update the session state.
        while (session_->pending())
        {
            do 
            {
                // wait for data or cancellation
                auto data   = socket_->wait(true, false);
                auto cancel = cancel_.wait();
                newsflash::wait(data, cancel);
                if (cancel) 
                    return;

                // readsome data
                const auto bytes = socket_->recvsome(buff.back(), buff.available());
                if (bytes == 0) 
                    throw exception("socket was closed unexpectedly", connection::error::network);

                // commit
                buff.append(bytes);
            }
            while (!session_->parse_next(buff, temp));
        }

        // check for errors
        const auto err = session_->get_error();
        if (err == session::error::authentication_rejected)
            throw connection::exception("authentication rejected", connection::error::authentication_rejected);
        else if (err == session::error::no_permission)
            throw connection::exception("no permission", connection::error::no_permission);
    }

    std::unique_ptr<socket> get_connected_socket()
    { return std::move(socket_); }

    std::unique_ptr<session> get_connected_session()
    { return std::move(session_); }

private:
    void do_auth(std::string&  user, std::string& pass) const
    { 
        user = username_;
        pass = password_;
    }
    void do_send(const std::string& cmd)
    {
        socket_->sendall(cmd.data(), cmd.size());
    }
private:
    std::unique_ptr<socket> socket_;
    std::unique_ptr<session> session_;
    std::string username_;
    std::string password_;
private:
    event& cancel_;
};

class connection::execute : public action
{
public:
    execute(cmdlist& list, session& ses, socket& sock, event& cancellation) : cmdlist_(list), session_(ses), socket_(sock), cancel_(cancellation)
    {
        session_.on_auth = [](std::string&, std::string&) { throw exception("no permission", connection::error::no_permission); };
        session_.on_send = std::bind(&execute::do_send, this,
            std::placeholders::_1);
        down_ = 0;
    }

    virtual void xperform() override
    {
        newsflash::buffer recvbuf(MB(4));

        // the cmdlist contains a list of commands
        // we pass the session object to the cmdlist to allow
        // it to submit a request into the session.
        // then while there are pending commands in the sesion
        // we read data from the socket into the buffer and pass 
        // it into the session in order to update the session state.
        // once a command is completed we pass the output buffer
        // to the cmdlist so that it can update its own state.

        do 
        {
            cmdlist_.submit(cmdlist::step::configure, session_);
            if (!session_.pending())
                break;

            newsflash::buffer config(KB(1));
            do 
            {
                auto data   = socket_.wait(true, false);
                auto cancel = cancel_.wait();
                if (!newsflash::wait_for(data, cancel, std::chrono::seconds(10)))
                    throw exception("socket timeout", connection::error::timeout);
                else if (cancel)
                    return;
            }
            while (!session_.parse_next(recvbuf, config));

            cmdlist_.receive(cmdlist::step::configure, config);
            cmdlist_.next(cmdlist::step::configure);
        }
        while (!cmdlist_.is_done(cmdlist::step::configure));

        if (!cmdlist_.is_good(cmdlist::step::configure))
            return;

        do 
        {
            cmdlist_.submit(cmdlist::step::transfer, session_);
            while (session_.pending())
            {
                newsflash::buffer content(MB(4));                
                do 
                {
                    // wait for data or cancellation
                    auto data   = socket_.wait(true, false);
                    auto cancel = cancel_.wait();
                    if (!newsflash::wait_for(data, cancel, std::chrono::seconds(10)))
                        throw exception("socket timeout", connection::error::timeout);
                    else if (cancel)
                        return;

                    // readsome
                    const auto bytes = socket_.recvsome(recvbuf.back(), recvbuf.available());
                    if (bytes == 0)
                        throw exception("socket was closed unexpectedly", connection::error::network);

                    down_ += bytes; // this includes header
                    recvbuf.append(bytes);
                }
                while (!session_.parse_next(recvbuf, content));

                // todo: is this oK? (in case when quota finishes..??)
                const auto err = session_.get_error();
                if (err != session::error::none)
                    throw exception("no permission", connection::error::no_permission);

                cmdlist_.receive(cmdlist::step::transfer, content);
                cmdlist_.next(cmdlist::step::transfer);
            }
        }
        while (!cmdlist_.is_done(cmdlist::step::transfer));
    }

    std::uint64_t downloaded_bytes() const 
    { return down_; }

private:
    void do_send(const std::string& cmd)
    {
        socket_.sendall(cmd.data(), cmd.size());
    }

private:
    cmdlist& cmdlist_;
private:
    session& session_;
    socket& socket_;
private:
    event& cancel_;
private:
    std::uint64_t down_;
};

class connection::disconnect : public action
{
public:
    disconnect(session& ses, socket& sock) : session_(ses), socket_(sock)
    {
        session_.on_send = std::bind(&disconnect::do_send, this,
            std::placeholders::_1);
    }

    virtual void xperform() override
    {
        newsflash::buffer buff(64);
        newsflash::buffer tmp(1);

        session_.quit();

        while (session_.pending())
        {
            do 
            {
                // wait for data for max 1s, if no response then kthx bye, whatever 
                // we're done anyway.
                auto data = socket_.wait(true, false);
                if (!newsflash::wait_for(data, std::chrono::seconds(1)))
                    return;

                const auto bytes = socket_.recvsome(buff.back(), buff.available());
                if (bytes == 0)
                    break;

                buff.append(bytes);
            }
            while (!session_.parse_next(buff, tmp));
        }
        LOG_D("Socket closed. Bye...");
    }
private:
    void do_send(const std::string& cmd)
    {
        socket_.sendall(cmd.data(), cmd.size());
    }

private:
    session& session_;
    socket& socket_;
};

connection::connection(std::size_t account, std::size_t id) : port_(0), ssl_(false)
{
    state_.st      = state::disconnected;
    state_.err     = error::none;
    state_.account = account;
    state_.id      = id;
    state_.task    = 0;
    state_.down    = 0;
    state_.host    = "";
    state_.desc    = "";
    state_.secure  = false;
    state_.bps     = 0;

    LOG_D("New connection ", id);
}

connection::~connection()
{
    LOG_D("Deleted connection ", state_.id);
}

void connection::connect(const connection::server& serv)
{
    cancellation_.reset();

    std::unique_ptr<newsflash::action> act(new resolve(serv.hostname));

    on_action(std::move(act));

    username_  = serv.username;
    password_  = serv.password;
    port_      = serv.port;
    ssl_       = serv.use_ssl;
    state_.st     = state::resolving;
    state_.err    = error::none;    
    state_.host   = serv.hostname;
    state_.secure = serv.use_ssl;
}

void connection::disconnect()
{
    assert(state_.st == state::connected);

    std::unique_ptr<newsflash::action> act(new class disconnect(*session_, *socket_));

    on_action(std::move(act));

    state_.st = state::disconnecting;
}

void connection::execute(std::string desc, std::size_t task, cmdlist* cmd)
{
    assert(state_.st == state::connected);

    cancellation_.reset();

    std::unique_ptr<newsflash::action> act(new class execute(*cmd, 
        *session_, *socket_, cancellation_));

    on_action(std::move(act));

    state_.st   = state::active;
    state_.err  = error::none;
    state_.task = task;
    state_.desc = desc;
}

void connection::complete(std::unique_ptr<action> act)
{
    // handle and translate any pending exception into appropriate error code
    // that the UI can represent
    if (act->has_exception())
    {
        std::error_code errc;
        std::string what;
        connection::error err;

        socket_.reset();
        session_.reset();
        state_.st = state::error;
        try
        {
            act->rethrow();
        }
        catch (const exception& e)
        {
            err  = e.error();
            what = e.what();
        }
        catch (const socket::tcp_exception& e)
        {
            errc = e.code();
            what = e.what();
            err  = error::network;            
        }
        catch (const std::system_error& e)
        {
            errc = e.code();
            what = e.what();            
            err  = error::other;
        }
        catch (const std::exception& e)
        {
            what = e.what();
            err  = error::other;
        }
        if (on_error)
            on_error({what, state_.host, errc});

        if (errc == std::errc::connection_refused)
            state_.err = error::refused;
        else state_.err = err;

        LOG_E("Connection error: ", what);
        if (errc != std::error_code())
            LOG_E("Error details: ", errc.value(), ", ", errc.message());

        return;
    }

    std::unique_ptr<action> next;

    // switch based on the current state and create a state transition
    // action for the following state (if any)
    switch (state_.st)
    {
        case state::resolving:
            if (cancellation_.is_set())
            {
                state_.st = state::disconnected;
            }
            else
            {
                const auto& action_resolve_host = dynamic_cast<resolve&>(*act);
                const auto resolved_address = action_resolve_host.get_resolved_address();
                next.reset(new class connect(cancellation_, resolved_address, port_, ssl_));
                state_.st = state::connecting;
            }
            break;

        case state::connecting:
            if (cancellation_.is_set())
            {
                state_.st = state::disconnected;
                socket_.reset();
                session_.reset();
            }
            else
            {
                auto& action_connect = dynamic_cast<class connect&>(*act);
                auto connected_socket = action_connect.get_connected_socket();
                next.reset(new class initialize(cancellation_, std::move(connected_socket), username_, password_));
                state_.st = state::initializing;
            }
            break;

        case state::initializing:
            if (cancellation_.is_set())
            {
                state_.st = state::disconnected;
                socket_.reset();
                session_.reset();
            }
            else
            {
                auto& action_init = dynamic_cast<class initialize&>(*act);
                socket_  = action_init.get_connected_socket();
                session_ = action_init.get_connected_session();

                state_.st = state::connected;                                        
            }
            break;

        case state::active:
            {
                auto& action = dynamic_cast<class execute&>(*act);

                state_.bps  = 0;
                state_.desc = "";
                state_.task = 0;
                state_.st   = state::connected;                
                state_.down += action.downloaded_bytes();
            }
            break;

        case state::disconnecting:
            state_.st = state::disconnected;
            break;

        case state::disconnected:
        case state::connected:
        case state::error:
            assert(!"wtufh");
            break;
    }

    if (next)
        on_action(std::move(next));
}

void connection::cancel()
{
    cancellation_.set();
}

} // newsflash
