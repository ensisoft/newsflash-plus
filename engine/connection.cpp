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
#include <fstream>
#include <cassert>
#include "connection.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "buffer.h"
#include "session.h"
#include "cmdlist.h"
#include "action.h"
#include "event.h"
#include "socketapi.h"

namespace newsflash
{

struct connection::state {
    std::mutex  mutex;
    std::string username;
    std::string password;
    std::string hostname;
    std::string logfile;
    std::ofstream log;
    std::uint16_t port;
    std::uint32_t addr;
    std::uint32_t bytes;
    std::unique_ptr<class socket> socket;
    std::unique_ptr<class session> session;
    std::unique_ptr<event> cancel;
    std::shared_ptr<cmdlist> cmds;
    bool ssl;
};

void connection::resolve::xperform()
{
    std::lock_guard<std::mutex> lock(state_->mutex);

    const auto hostname = state_->hostname;

    LOG_D("Resolving hostname ", hostname);

    std::uint32_t addr;

    const auto err = resolve_host_ipv4(hostname, addr);
    if (err)
    {
        LOG_E("Failed to resolve host (", err.value(), ") ", err.message());
        throw exception(error::resolve, "resolve failed");
    }
    state_->addr = addr;

    LOG_D("Hostname resolved to ", ipv4{state_->addr});
}

connection::connect::connect(std::shared_ptr<state> s) : state_(s)
{
    if (state_->ssl)
        state_->socket.reset(new sslsocket);
    else state_->socket.reset(new tcpsocket);
}

void connection::connect::xperform()
{
    std::lock_guard<std::mutex> lock(state_->mutex);

    LOG_D("Connecting to ", ipv4{state_->addr}, ", ", state_->port);

    const auto& socket = state_->socket;
    const auto& cancel = state_->cancel;
    const auto addr = state_->addr;
    const auto port = state_->port;

    // first begin connection (async)
    socket->begin_connect(addr, port);

    // wait for completion or cancellation event.
    auto connected = socket->wait(true, false);
    auto canceled  = cancel->wait();
    if (!newsflash::wait_for(connected, canceled, std::chrono::seconds(10)))
        throw exception(error::timeout, "connection attempt timed out");
    if (canceled)
    {
        LOG_D("Connection was canceled");
        return;
    }

    const auto err = socket->complete_connect();    
    if (err)
        throw std::system_error(err, "connect failed");

    LOG_D("Socket connection ready!");
}

connection::initialize::initialize(std::shared_ptr<state> s) : state_(s)
{
    state_->session.reset(new session);
    state_->session->on_auth = [=](std::string& user, std::string& pass) {
        if (state_->username.empty() || state_->password.empty())
            throw exception(error::no_permission, "authentication required but no credentials provided");
        user = state_->username;
        pass = state_->password;
    };

    state_->session->on_send = [=](const std::string& cmd) {
        state_->socket->sendall(&cmd[0], cmd.size());
    };
}

void connection::initialize::xperform()
{
    std::lock_guard<std::mutex> lock(state_->mutex);

    LOG_D("Initializing NNTP session");

    auto& session = state_->session;
    auto& socket  = state_->socket;
    auto& cancel  = state_->cancel;

    // begin new session
    session->start();

    newsflash::buffer buff(1024);
    newsflash::buffer temp;

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
        while (!session->parse_next(buff, temp));
    }    

    // check for errors
    const auto err = session->get_error();

    if (err == session::error::authentication_rejected)
        throw exception(connection::error::authentication_rejected, "authentication rejected");
    else if (err == session::error::no_permission)
        throw exception(connection::error::no_permission, "no permission");

    LOG_D("NNTP Session ready");
}

connection::execute::execute(std::shared_ptr<state> s) : state_(s)
{
    state_->session->on_auth = [](std::string&, std::string&) { 
        throw exception(connection::error::no_permission, 
            "unexpected authentication requested"); 
    };
        
    state_->session->on_send = [=](const std::string& cmd) {
        state_->socket->sendall(&cmd[0], cmd.size());
    };
            
    state_->bytes = 0;    
}

void connection::execute::xperform()
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
    // then while there are pending commands in the session
    // we read data from the socket into the buffer and pass 
    // it into the session in order to update the session state.
    // once a command is completed we pass the output buffer
    // to the cmdlist so that it can update its own state.

    bool configure_success = false;

    for (std::size_t i=0; ; ++i)
    {
        if (!cmdlist->submit_configure_command(i, *session))
            break;

        newsflash::buffer config(KB(1));
        do 
        {
            auto received = socket->wait(true, false);
            auto canceled = cancel->wait();
            if (!newsflash::wait_for(received, canceled, std::chrono::seconds(10)))
                throw exception(connection::error::timeout, "connection timeout");
            else if (cancel)
                return;

            const auto bytes = socket->recvsome(recvbuf.back(), recvbuf.available());
            if (bytes == 0)
                throw exception(connection::error::network, "connection was closed unexpectedly");

            recvbuf.append(bytes);
        }
        while (!session->parse_next(recvbuf, config));

        if (cmdlist->receive_configure_buffer(i, std::move(config)))
        {
            configure_success = true;
            break;
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
                throw exception(connection::error::timeout, "connection timeout");
            else if (cancel)
                return;

            // readsome
            const auto bytes = socket->recvsome(recvbuf.back(), recvbuf.available());
            if (bytes == 0)
                throw exception(connection::error::network, "connection was closed unexpectedly");

            state_->bytes += bytes; // this includes header data
            recvbuf.append(bytes);
        }
        while (!session->parse_next(recvbuf, content));

        // todo: is this oK? (in case when quota finishes..??)
        const auto err = session->get_error();
        if (err != session::error::none)
            throw exception(connection::error::no_permission, "no permission");

        cmdlist->receive_data_buffer(std::move(content));
    }    
}

connection::disconnect::disconnect(std::shared_ptr<state> s) : state_(s)
{
    state_->session->on_send = [=](const std::string& cmd) {
        state_->socket->sendall(&cmd[0], cmd.size());
    };
}

void connection::disconnect::xperform()
{
    std::lock_guard<std::mutex> lock(state_->mutex);

    LOG_D("Perform disconnect");

    auto& session = state_->session;
    auto& socket  = state_->socket;

    newsflash::buffer buff(64);
    newsflash::buffer temp;

    session->quit();
    do 
    {
        // wait for data, if no response then khtx bye whatever, we're done anyway
        auto received = socket->wait(true, false);
        if (!newsflash::wait_for(received, std::chrono::seconds(1)))
            break;

        const auto bytes = socket->recvsome(buff.back(), buff.available());
        if (bytes == 0)
        {
            LOG_D("Received socket close");
            break;
        }
        buff.append(bytes);
    }
    while (!session->parse_next(buff, temp));

    socket->close();

    LOG_D("Disconnect complete");
}

connection::ping::ping(std::shared_ptr<state> s) : state_(s)
{
    state_->session->on_send = [=](const std::string& cmd) {
        state_->socket->sendall(&cmd[0], cmd.size());
    };
}

void connection::ping::xperform() 
{
    std::lock_guard<std::mutex> lock(state_->mutex);

    LOG_D("Perform ping");

    auto& session = state_->session;
    auto& socket  = state_->socket;

    newsflash::buffer buff(64);
    newsflash::buffer temp;

    // NNTP doesn't have a "real" ping built into it
    // so we simply send a mundane command (change group)
    // to perform some activity on the transmission line.
    session->change_group("keeping.session.alive");

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
    while (!session->parse_next(buff, temp));


}

std::unique_ptr<action> connection::connect(spec s)
{
    state_ = std::make_shared<state>();
    state_->username = s.username;
    state_->password = s.password;
    state_->hostname = s.hostname;
    state_->port     = s.hostport;
    state_->addr     = 0;
    state_->bytes    = 0;
    state_->ssl      = s.use_ssl;
    state_->cancel.reset(new event);
    state_->cancel->reset();
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
    auto* ptr = a.get();

    std::unique_ptr<action> next;

    if (dynamic_cast<resolve*>(ptr))
    {
        next.reset(new class connect(state_));
    }
    else if (dynamic_cast<class connect*>(ptr))
    {
        next.reset(new class initialize(state_));
    }
    return next;
}

std::unique_ptr<action> connection::execute(std::shared_ptr<cmdlist> cmd)
{
    assert(!state_->cmds);

    state_->cmds = cmd;
    state_->cancel->reset();

    std::unique_ptr<action> act(new class execute(state_));

    return act;
}

void connection::cancel()
{
    state_->cancel->set();
}

const std::string& connection::username() const 
{ return state_->username; }

const std::string& connection::password() const 
{ return state_->password; }


} // newsflash
