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
#include "event.h"
#include "cmdlist.h"

namespace newsflash
{


struct connection_state {
    std::unique_ptr<newsflash::socket> socket;
    std::unique_ptr<newsflash::session> session;
    bool run_thread;
    event cancel;
};

class connection::exception : public std::exception
{
public:
    exception(std::string what, connection::error err) : what_(std::move(what)), error_(err)
    {}

    const char* what() const noexcept
    { return what_.c_str(); }

    connection::error error() const noexcept
    { return error_; }
private:
    std::string what_;
    connection::error error_;
};


class connection::action 
{
public:
    virtual ~action() = default;

    virtual void perform(connection_state& st) = 0;
protected:
private:
};


class connection::connect : public connection::action
{
public:
    connect(std::string username, std::string password, std::string host, std::uint16_t port, bool use_ssl)
        : username_(std::move(username)), password_(std::move(password)), hostname_(std::move(host)), port_(port)
    {
        if (use_ssl)
            socket_.reset(new sslsocket);
        else socket_.reset(new tcpsocket);

        session_.reset(new session);
        session_->on_auth = std::bind(&connect::do_auth, this,
            std::placeholders::_1, std::placeholders::_2);
        session_->on_send = std::bind(&connect::do_send, this,
            std::placeholders::_1);        
    }

    virtual void perform(connection_state& st) override
    {
        LOG_I("Resolving host address... (", hostname_, ":", port_, ")");
        const auto addr = resolve_host_ipv4(hostname_);
        if (!addr)
            throw connection::exception("resolve host failed", connection::error::resolve_host);

        LOG_D("Connecting...");
        socket_->begin_connect(addr, port_);

        // wait shit be here
        auto connect = socket_->wait(true, false);
        auto cancel  = st.cancel.wait();
        newsflash::wait(connect, cancel);
        if  (cancel)
        {
            LOG_D("Connection was canceled");
            return;
        }

        socket_->complete_connect();

        LOG_D("Initialize session...");

        // initialize the session
        newsflash::buffer buffer(1024);
        newsflash::buffer tmp(1);

        while (session_->pending())
        {
            while (!session_->parse_next(buffer, tmp))
            {
                const auto bytes = socket_->recvsome(buffer.back(), buffer.available());
                if (bytes == 0)
                    throw connection::exception("connection was closed", connection::error::other);
                buffer.append(bytes);
            }
        }

        LOG_I("Connection established!");

        st.session = std::move(session_);
        st.socket  = std::move(socket_);
    }
private:
    void do_auth(std::string& user, std::string& pass)
    { 
        user = username_;
        pass = password_;
    }
    void do_send(const std::string& cmd)
    {}
private:
    std::string username_;
    std::string password_;
    std::string hostname_;
    std::uint16_t port_;
private:
    std::unique_ptr<socket> socket_;
    std::unique_ptr<session> session_;
};

class connection::shutdown : public connection::action
{
public:
    void perform(connection_state& state) override
    {
        state.run_thread = false;
    }
private:
};

class connection::execute : public connection::action
{
public:
    void perform(connection_state& state) override
    {
        auto& session = *state.session;
        auto& socket  = *state.socket;

        session.on_send = std::bind(&execute::do_send, this,
            std::placeholders::_1, std::ref(socket));

        newsflash::buffer buffer(MB(4));
        newsflash::buffer payload(MB(4));

        while (!cmdlist_->is_done())
        {
            cmdlist_->submit_command(session);

            while (session.pending())
            {
                while (!session.parse_next(buffer, payload))
                {
                    if (buffer.full())
                        throw connection::exception("out of buffer space", connection::error::other);
                    const auto bytes = socket.recvsome(buffer.back(), buffer.available());
                    if (bytes == 0)
                        throw connection::exception("connection was closed", connection::error::other);
                    buffer.append(bytes);
                }
                cmdlist_->receive(payload);                
                cmdlist_->next();                
            }
        }
    }
private:
    void do_send(const std::string& cmd, newsflash::socket& socket)
    {
        socket.sendall(cmd.data(), cmd.size());
    }
private:
    cmdlist* cmdlist_;
};

connection::connection(std::string logfile)
{
    thread_.reset(new std::thread(
        std::bind(&connection::thread_loop, this, std::move(logfile))));
}

connection::~connection() 
{}

void connection::shutdown()
{
    std::unique_ptr<action> ptr(new class shutdown);

    std::lock_guard<std::mutex> lock(mutex_);
    actions_.push(std::move(ptr));
    cond_.notify_one();
}

void connection::connect(std::string username, std::string password, 
    std::string hostname, std::uint16_t port, bool ssl)
{
    std::unique_ptr<action> ptr(new class connect(
        username, password, hostname, port, ssl));

    std::lock_guard<std::mutex> lock(mutex_);
    actions_.push(std::move(ptr));
    cond_.notify_one();
}

void connection::thread_loop(std::string logfile)
{
    LOG_OPEN("Connection", logfile);

    connection_state cstate;
    cstate.run_thread = true;

    while (cstate.run_thread)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (actions_.empty())
            cond_.wait(lock);

        if (actions_.empty())
            continue;

        auto action = std::move(actions_.front());
        actions_.pop();

        try
        {
            action->perform(cstate);
        }
        catch (const socket::tcp_exception& e)
        {}
        catch (const socket::ssl_exception& e)
        {}
        catch (const connection::exception& e)
        {}
        catch (const std::exception& e)
        {}
    }

    LOG_CLOSE();
}

} // newsflash
