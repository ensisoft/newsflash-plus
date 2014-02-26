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

#include <boost/lexical_cast.hpp>
#include <thread>
#include <functional>
#include "connection.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "socketapi.h"
#include "logging.h"
#include "buffer.h"
#include "platform.h"

namespace {
    class conn_exception : public std::exception 
    {
    public:
        conn_exception(std::string what, newsflash::connection::error e) 
           : what_(std::move(what)), 
             error_(e)
        {}
        const char* what() const NOTHROW
        {
            return what_.c_str();
        }
        newsflash::connection::error err() const NOTHROW
        {
            return error_;
        }
    private:
        const std::string what_;
        const newsflash::connection::error error_;
    };

} // namespace

namespace newsflash
{

connection::connection(const std::string& logfile, const server& host, cmdqueue& cmd, cmdqueue& res) 
    : state_(state::connecting), 
      error_(error::none),
      bytes_(0),
      speed_(0),
      commands_(cmd), responses_(res)
{
    thread_.reset(new std::thread(std::bind(&connection::main, this, host, logfile)));
}


connection::~connection()
{
    shutdown_.set();
    thread_->join();
}

connection::status connection::get_status()
{
    connection::status ret;

    std::lock_guard<std::mutex> lock(mutex_);

    ret.state = state_;
    ret.error = error_;
    ret.bytes = bytes_;
    ret.bps   = speed_;

    status_.reset();
    return ret;
}

waithandle connection::wait() const 
{
    return status_.wait();
}

void connection::main(const server& host, const std::string& logfile)
{
    LOG_OPEN(logfile);

    LOG_D("Connection thread ", std::this_thread::get_id());

    try
    {
        connect(host);

        LOG_FLUSH();

        const std::chrono::seconds ping_interval(10);            
        while (true)
        {
            goto_state(state::idle, error::none);
             
            auto execcmd  = commands_.wait();
            auto shutdown = shutdown_.wait();
            if (!newsflash::wait_for(execcmd, shutdown, ping_interval))
            {
                ping();
            }
            else if (execcmd)
            {
                execute();
            }
            else if (shutdown)
            {
                break;
            }

            LOG_FLUSH();
        } 
        quit();
    }
    catch (const conn_exception& e)
    {
        goto_state(state::error, e.err());

        LOG_E(e.what());
    }
    catch (const protocol::exception& e)
    {
        const auto err = e.code();
        if (err == protocol::error::authentication_failed)
             goto_state(state::error, error::forbidden);
        else goto_state(state::error, error::protocol);

        LOG_E(e.what());
    }
    catch (const socket::tcp_exception& e)
    {
        const auto err = e.code();
        if (err == std::errc::connection_refused)
            goto_state(state::error, error::refused);
        else goto_state(state::error, error::socket);

        LOG_E(e.what());
    }
    catch (const socket::ssl_exception& e)
    {
        goto_state(state::error, error::ssl);

        LOG_E(e.what());
    }
    catch (const std::exception& e)
    {
        goto_state(state::error, error::unknown);

        LOG_E(e.what());
    }


    LOG_D("Thread exiting.");

    LOG_CLOSE();
}

void connection::connect(const server& host)
{
    LOG_I("Connecting to ", host.addr, ":", host.port);

    const auto addr = resolve_host_ipv4(host.addr);
    if (addr == 0)
        throw conn_exception("failed to resolve host", error::resolve);

    LOG_I("Resolved to ", format_ipv4(addr));
    LOG_I("Using SSL ", host.ssl);

    if (host.ssl) 
        socket_.reset(new sslsocket());
    else socket_.reset(new tcpsocket());

    LOG_D("Begin connect...");
    socket_->begin_connect(addr, host.port);

    auto connect  = socket_->wait();
    auto shutdown = shutdown_.wait();
    newsflash::wait_for(connect, shutdown);

    if (shutdown)
        throw conn_exception("connect was interrupted", error::interrupted);

    socket_->complete_connect();


    LOG_D("Socket connection established.");
    
    username_ = host.user;
    password_ = host.pass;

    // socket is connected state, next initialize the protocol stack.
    proto_.on_recv = std::bind(&connection::read, this, std::placeholders::_1, std::placeholders::_2);
    proto_.on_send = std::bind(&connection::send, this, std::placeholders::_1, std::placeholders::_2);
    proto_.on_auth = std::bind(&connection::auth, this, std::placeholders::_1, std::placeholders::_2);
    proto_.on_log  = [](const std::string& str) { LOG_I(str); };

    LOG_D("Connect protocol");
    proto_.connect();

    LOG_I("Connection ready!");
}

void connection::execute()
{
    auto cmd = commands_.try_get_front();
    if (!cmd)
        return;

    goto_state(state::active, error::none);
    try
    {
        cmd->perform(proto_);

        responses_.push_back(std::move(cmd));
    }
    catch (const std::exception&)
    {
        // presumably we have a problem with socket IO or SSL
        // so store the command back in the command queue so it
        // won't get lost.
        commands_.push_front(std::move(cmd));
        throw;
    }
}

void connection::ping()
{
    proto_.ping();
}

void connection::quit()
{
    proto_.quit();
}

void connection::auth(std::string& user, std::string& pass)
{
    LOG_I("Authenticating...");
    user = username_;
    pass = password_;
}

void connection::send(const void* data, size_t len)
{
    // the commands that we send are miniscule so just do it the simple way.
    // this shouldn't be a problem, but is interruption a problem?
    socket_->sendall(data, (int)len);
}

size_t connection::read(void* buff, size_t len)
{
    const std::chrono::seconds timeout(15);

    // if we don't receive data in "timeout" seconds, we can consider
    // that the connection has timed out and become stale.
    // note that the protocol stack expects this function
    // to return the number of bytes > 0. so in case of error we must throw.
    auto sockread = socket_->wait(true, false);
    auto shutdown = shutdown_.wait();

    if (!newsflash::wait_for(sockread, shutdown, timeout))
        throw conn_exception("socket timeout", error::timeout);

    if (shutdown)
        throw conn_exception("read interrupted", error::interrupted);

    // todo: throttle
    // todo: handle 0 recv when shutting down connection (socket closed)

    const size_t ret = socket_->recvsome(buff, (int)len);

    speed_ = meter_.submit(ret);
    return ret;
}

void connection::goto_state(state s, error e)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (s == state_)
        return;
    state_ = s;
    error_ = e;

    switch (state_)
    {
        case state::active:
            meter_.start();
            break;

        case state::idle:
            meter_.end();
            break;

        default:  
            break;
    }

    // set a signal indicating that the status has changed
    status_.set();
}

} // newsflash
