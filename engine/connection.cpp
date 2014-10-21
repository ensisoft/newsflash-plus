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

#include <mutex>
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

struct connection::privstate {
    std::mutex  mutex;
    std::string username;
    std::string password;
    std::string hostname;
    std::uint16_t port;
    std::uint32_t addr;
    std::uint32_t bytes;
    std::unique_ptr<class socket> socket;
    std::unique_ptr<class session> session;
    std::unique_ptr<event> cancel;
    std::shared_ptr<cmdlist> cmds;
    bool ssl;
};

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
    resolve(std::shared_ptr<privstate> state) : state_(state)
    {}

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        const auto hostname = state_->hostname;

#if defined(LINUX_OS)
        struct addrinfo* addrs = nullptr;
        const auto ret = getaddrinfo(hostname.c_str(), nullptr, nullptr, &addrs);
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
        state_->addr = ntohl(host);

#elif defined(WINDOWS_OS)
        const auto* hostent = gethostbyname(hostname.c_str());
        if (hostent == nullptr)
        {
            throw exception("resolve host error", std::error_code(WSAGetLastError(), std::system_category()),
                connection::error::resolve);
        }
        const auto* addr = static_cast<const in_addr*>((void*)hostent->h_addr);
        state_->addr = ntohl(addr->s_addr);
#endif
    }
private:
    std::shared_ptr<privstate> state_;
};

// try to connect a socket to the given resolved host address and port
class connection::connect : public action
{
public:
    connect(std::shared_ptr<privstate> state) : state_(state)
    {
        if (state->ssl)
            state_->socket.reset(new sslsocket);
        else state->socket.reset(new tcpsocket);
    }

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        const auto& socket = state_->socket;
        const auto& cancel = state_->cancel;
        const auto addr = state_->addr;
        const auto port = state_->port;

        // first begin connection (async)
        socket->begin_connect(addr, port);

        // wait for completion or cancellation event.
        auto connected = socket->wait(true, false);
        auto canceled  = cancel->wait();
        newsflash::wait(connected, canceled);
        if (canceled)
            return;

        // complete connect, throws on error
        socket->complete_connect();
    }
private:
    std::shared_ptr<privstate> state_;
};

// initialize the session object
class connection::initialize : public action
{
public:
    initialize(std::shared_ptr<privstate> state) : state_(state)
    {
        state_->session.reset(new session);
        state_->session->on_auth = std::bind(&initialize::do_auth, this, 
            std::placeholders::_1, std::placeholders::_2);
        state_->session->on_send = std::bind(&initialize::do_send, this,
            std::placeholders::_1);
    }
    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        auto& session = state_->session;
        auto& socket  = state_->socket;
        auto& cancel  = state_->cancel;

        // begin new session
        session->start();

        newsflash::buffer buff(1024);
        newsflash::buffer temp(1);

        // while there are pending commands in the session we read 
        // data from the socket into the buffer and then feed the buffer
        // into the session to update the session state.
        while (session->pending())
        {
            do 
            {
                // wait for data or cancellation
                auto received = socket->wait(true, false);
                auto canceled = cancel->wait();
                newsflash::wait(received, canceled);
                if (canceled) 
                    return;

                // readsome data
                const auto bytes = socket->recvsome(buff.back(), buff.available());
                if (bytes == 0) 
                    throw exception("socket was closed unexpectedly", connection::error::network);

                // commit
                buff.append(bytes);
            }
            while (!session->parse_next(buff, temp));
        }

        // check for errors
        const auto err = session->get_error();
        if (err == session::error::authentication_rejected)
            throw connection::exception("authentication rejected", connection::error::authentication_rejected);
        else if (err == session::error::no_permission)
            throw connection::exception("no permission", connection::error::no_permission);
    }
private:
    void do_auth(std::string&  user, std::string& pass) const
    { 
        user = state_->username;
        pass = state_->password;
    }
    void do_send(const std::string& cmd)
    {
        state_->socket->sendall(cmd.data(), cmd.size());
    }
private:
    std::shared_ptr<privstate> state_;
};

class connection::execute : public action
{
public:
    execute(std::shared_ptr<privstate> state) : state_(state)
    {
        auto& session = state->session;

        session->on_auth = [](std::string&, std::string&) { throw exception("no permission", connection::error::no_permission); };
        session->on_send = std::bind(&execute::do_send, this,
            std::placeholders::_1);
        state->bytes = 0;
    }

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        auto& session = state_->session;
        auto& cancel  = state_->cancel;
        auto& socket  = state_->socket;
        auto& cmdlist = state_->cmds;

        newsflash::buffer recvbuf(MB(4));

        // the cmdlist contains a list of commands
        // we pass the session object to the cmdlist to allow
        // it to submit a request into the session.
        // then while there are pending commands in the sesion
        // we read data from the socket into the buffer and pass 
        // it into the session in order to update the session state.
        // once a command is completed we pass the output buffer
        // to the cmdlist so that it can update its own state.

        bool configure_success = false;

        for (std::size_t i=0; ; ++i)
        {
            if (cmdlist->submit_configure_command(i, *session))
            {
                newsflash::buffer config(KB(1));
                do 
                {
                    auto received = socket->wait(true, false);
                    auto canceled = cancel->wait();
                    if (!newsflash::wait_for(received, canceled, std::chrono::seconds(10)))
                        throw exception("socket timeout", connection::error::timeout);
                    else if (cancel)
                        return;

                    const auto bytes = socket->recvsome(recvbuf.back(), recvbuf.available());
                    if (bytes == 0)
                        throw exception("socket was closed unexpectedly", connection::error::network);

                    recvbuf.append(bytes);
                }
                while (!session->parse_next(recvbuf, config));

                if (cmdlist->receive_configure_buffer(i, std::move(config)))
                {
                    configure_success = true;
                    break;
                }
            }
        }
        if (!configure_success)
            return;

        cmdlist->submit_data_commands(*session);

        while (session->pending())
        {
            newsflash::buffer content(MB(4));                
            do 
            {
                // wait for data or cancellation
                auto received = socket->wait(true, false);
                auto canceled = cancel->wait();
                if (!newsflash::wait_for(received, canceled, std::chrono::seconds(10)))
                    throw exception("socket timeout", connection::error::timeout);
                else if (cancel)
                    return;

                // readsome
                const auto bytes = socket->recvsome(recvbuf.back(), recvbuf.available());
                if (bytes == 0)
                    throw exception("socket was closed unexpectedly", connection::error::network);

                state_->bytes += bytes; // this includes header data
                recvbuf.append(bytes);
            }
            while (!session->parse_next(recvbuf, content));

            // todo: is this oK? (in case when quota finishes..??)
            const auto err = session->get_error();
            if (err != session::error::none)
                throw exception("no permission", connection::error::no_permission);

            cmdlist->receive_data_buffer(std::move(content));
        }
    }
private:
    void do_send(const std::string& cmd)
    {
        state_->socket->sendall(cmd.data(), cmd.size());
    }

private:
    std::shared_ptr<privstate> state_;
};

class connection::disconnect : public action
{
public:
    disconnect(std::shared_ptr<privstate> state) : state_(state)
    {
        state->session->on_send = std::bind(&disconnect::do_send, this,
            std::placeholders::_1);
    }

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);

        newsflash::buffer buff(64);
        newsflash::buffer tmp(1);

        auto& session = state_->session;
        auto& socket  = state_->socket;
        auto& cancel  = state_->cancel;

        session->quit();

        while (session->pending())
        {
            do 
            {
                // wait for data for max 1s, if no response then kthx bye, whatever 
                // we're done anyway.
                auto received = socket->wait(true, false);
                if (!newsflash::wait_for(received, std::chrono::seconds(1)))
                    return;

                const auto bytes = socket->recvsome(buff.back(), buff.available());
                if (bytes == 0)
                    break;

                buff.append(bytes);
            }
            while (!session->parse_next(buff, tmp));
        }
        LOG_D("Socket closed. Bye...");
    }
private:
    void do_send(const std::string& cmd)
    {
        state_->socket->sendall(cmd.data(), cmd.size());
    }

private:
    std::shared_ptr<privstate> state_;
};

connection::connection(const connection::spec& s)
{
    uistate_.st      = state::disconnected;
    uistate_.err     = error::none;
    uistate_.account = s.account;
    uistate_.id      = s.id;
    uistate_.task    = 0;
    uistate_.down    = 0;
    uistate_.host    = s.hostname;
    uistate_.port    = s.port;
    uistate_.desc    = "";
    uistate_.secure  = s.use_ssl;
    uistate_.bps     = 0;

    state_ = std::make_shared<privstate>();
    state_->hostname = s.hostname;
    state_->username = s.username;
    state_->password = s.password;    
    state_->port     = s.port;
    state_->addr     = 0;    
    state_->bytes    = 0;
    state_->ssl      = s.use_ssl;
    state_->cancel.reset(new event);

    LOG_D("Connection created");
}

connection::~connection()
{
    LOG_D("Connection deleted");
}

void connection::connect()
{
    assert(uistate_.st == state::disconnected);

    // UI state
    uistate_.st  = state::resolving;
    uistate_.err = error::none;

    // begin connect sequence with host resolve.
    std::unique_ptr<action> act(new resolve(state_));
    act->set_affinity(action::affinity::any_thread);
    act->set_id(uistate_.id);

    on_action(std::move(act));

    LOG_D("Connecting");
}

void connection::disconnect()
{
    // disconnect can happen in any state.
    // the idea is that the calling code can just say "disconnect" and then
    // throw away the connection object and the connection
    // can siletly perform socket closure on the background.


    if (uistate_.st == state::disconnected ||
        uistate_.st == state::error ||
        uistate_.st == state::disconnecting)
        return;

    if (uistate_.st == state::resolving ||
        uistate_.st == state::connecting)
    {
        state_->cancel->set();
        uistate_.st = state::disconnected;
        return;
    }

    // perform clean shutdown
    std::unique_ptr<action> act(new class disconnect(state_));
    act->set_affinity(action::affinity::single_thread);
    act->set_id(uistate_.id);

    if (uistate_.st == state::active)
    {
        state_->cancel->set();
    }

    uistate_.st = state::disconnecting;
    on_action(std::move(act));

    LOG_D("Disconnecting");
}

void connection::execute(std::shared_ptr<cmdlist> cmd, std::string desc, std::size_t task)
{
    assert(uistate_.st == state::connected);
    assert(uistate_.err == error::none);

    state_->cancel->reset();
    state_->cmds = cmd;

    uistate_.st   = state::active;
    uistate_.task = task;
    uistate_.desc = desc;

    std::unique_ptr<action> act(new class execute(state_));
    act->set_affinity(action::affinity::single_thread);
    act->set_id(uistate_.id);

    on_action(std::move(act));

    LOG_D("Execute cmdlist ", desc);
}

void connection::complete(std::unique_ptr<action> act)
{
    std::unique_lock<std::mutex> lock(state_->mutex);

    // handle and translate any pending exception into appropriate error code
    // that the UI can represent
    if (act->has_exception())
    {
        std::error_code errc;
        std::string what;
        connection::error err;

        state_.reset();
        uistate_.st   = state::error;
        uistate_.bps  = 0;
        uistate_.desc = "";
        uistate_.task = 0;

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
            on_error({what, uistate_.host, errc});

        if (errc == std::errc::connection_refused)
            uistate_.err = error::refused;
        else uistate_.err = err;

        LOG_E("Connection error: ", what, ", ", errc.value(), ", ", errc.message());
        return;
    }

    std::unique_ptr<action> next;

    // if there was no error then perform state change here.
    switch (uistate_.st)
    {
        case state::resolving: 
            next.reset(new class connect(state_));
            uistate_.st = state::connecting;
            break;

        case state::connecting:
            next.reset(new class initialize(state_));
            uistate_.st = state::initializing;
            break;

        case state::initializing:
            uistate_.st = state::connected;
            break;

        case state::active:
            uistate_.st   = state::connected;
            uistate_.desc = "";
            uistate_.task = 0;
            uistate_.bps  = 0;
            state_->cmds.reset();
            break;

        case state::disconnecting:
            if (auto* ptr = dynamic_cast<class disconnect*>(act.get()))
                uistate_.st  = state::disconnected;
            break;

            // we're ignoring this action if it ever gets dispatched back here.
            // it's now obsolete since we're already disconnected.
        case state::disconnected:
            break;

        default:
           assert(!"wut");
           break;
    }

    if (next)
        on_action(std::move(next));
}

std::string connection::username() const 
{ return state_->username; }

std::string connection::password() const 
{ return state_->password; }

ui::connection connection::get_ui_state() const
{
    return uistate_;
}

} // newsflash
