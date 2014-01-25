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

#include <thread>
#include <functional>
#include "connection.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "socketapi.h"
#include "logging.h"

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

connection::connection(const std::string& logfile, const server& host, cmdqueue& in, resqueue& out) 
    : state_(state::connecting), 
      error_(error::none),
      bytes_(0),
      speed_(0),
      commands_(in), responses_(out),
      thread_(std::bind(&connection::main, this, host, logfile))
{}

connection::~connection()
{
    shutdown_.set();
    thread_.join();
}

void connection::main(const server& host, const std::string& logfile)
{
    LOG_OPEN(logfile);
    LOG_D("Connection thread: ", std::this_thread::get_id());

    const std::chrono::seconds ping_interval(10);

    try
    {
        if (!connect(host))
            return;

        while (true)
        {
            goto_state(state::idle, error::none);

            // wait for commands or shutdown signal
            auto command_handle  = commands_.wait();
            auto shutdown_handle = shutdown_.wait();

            if (!wait(command_handle, shutdown_handle, ping_interval))
            {
                proto_.ping();
            }
            else if (shutdown_handle.read())
            {
                LOG_D("Shutdown handle signaled");
                break;            
            }
            else if (command_handle.read())
            {
                LOG_D("Command handle signaled");
                execute();
            }
        }

        proto_.quit();
    }
    catch (const conn_exception& e)
    {
        goto_state(state::error, e.err());

        LOG_E(e.what());
    }
    catch (const protocol::exception& e)
    {
        const auto info = e.error();
        if (info == protocol::exception::code::authentication_failed)
             goto_state(state::error, error::forbidden);
        else goto_state(state::error, error::protocol);

        LOG_E(e.what());
    }
    catch (const socket::io_exception& e)
    {
        goto_state(state::error, error::socket);

        LOG_E(e.what());
    }
    catch (const socket::ssl_exception& e)
    {
        goto_state(state::error, error::ssl);

        LOG_E(e.what());
    }
    LOG_D("Thread exiting...");
}

bool connection::connect(const server& host)
{
    LOG_I("Connecting to: ", host.addr, ":", host.port);

    auto addr = resolve_host_ipv4(host.addr);
    if (addr == 0)
    {
        LOG_E("Failed to resolve host");
        goto_state(state::error, error::resolve);
        return false;
    }
    if (host.ssl) 
         socket_.reset(new sslsocket());
    else socket_.reset(new tcpsocket());

    LOG_D("Begin connect...");
    socket_->connect(addr, host.port);

    auto socket_handle   = socket_->wait();
    auto shutdown_handle = shutdown_.wait();
    wait(socket_handle, shutdown_handle);

    if (shutdown_handle.read())
    {
        // if shutdown is signaled we're being shutdown while we're connecting.
        // in that case, throw away the socket and return false.
        LOG_D("Connect was interrupted.");
        return false;
    }
    else if (socket_handle.read())
    {
        struct log_helper {
            static void stub(const std::string& str)
            {
                LOG_I(str);
            }
        };

        LOG_D("Socket connection established.");
        // if socket is ready to read we're connected
        // initialize protocol stack
        proto_.on_recv = std::bind(&connection::read, this, std::placeholders::_1, std::placeholders::_2);
        proto_.on_send = std::bind(&connection::send, this, std::placeholders::_1, std::placeholders::_2);
        proto_.on_log  = log_helper::stub;
        proto_.connect();
    }

    LOG_I("Connection ready!");
    return true;
}

void connection::execute()
{
    command cmd;
    if (!commands_.get_one(cmd))
        return;

    goto_state(state::active, error::none);

    try
    {
        switch (cmd.kind)
        {
            case command::type::body:
            {
                //proto_.change_group(cmd.group);
               //proto_.download_article(cmd.)
            }
            break;

            case command::type::xover:
            {
                proto_.change_group(cmd.group);

            }
            break;

            case command::type::list:
            {

            }
            break;
        }
    }
    catch (const std::exception& e)
    {
        // presumably we have a problem with socket IO or SSL
        // but we must make sure not to loose the command data...
        commands_.try_again_later(cmd);
        throw;
    }
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

    // if we don't receive data in 15 seconds, we can consider
    // that the connection has timed out and become stale.
    // note that the protocol stack expects this function
    // to return the number of bytes > 0. so in case of
    // error we must throw.
    auto socket_handle   = socket_->wait(true, false);
    auto shutdown_handle = shutdown_.wait();

    if (!wait(socket_handle, shutdown_handle, timeout))
        throw conn_exception("socket timeout", error::timeout);

    if (shutdown_handle.read())
        throw conn_exception("read interrupted", error::interrupted);

    assert(socket_handle.read());

    // todo: throttle
    // todo: speedometer

    const size_t ret = socket_->recvsome(buff, len);

    return ret;
}

void connection::goto_state(state s, error e)
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = s;
    error_ = e;
}

} // newsflash
