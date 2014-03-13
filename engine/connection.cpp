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
#include "platform.h"
#include "protocol.h"
#include "cmdlist.h"
#include "event.h"

namespace {
    using namespace newsflash;

    class conn_exception : public std::exception 
    {
    public:
        conn_exception(std::string what, connection::error e) 
           : what_(std::move(what)), 
             error_(e)
        {}
        const char* what() const NOTHROW
        {
            return what_.c_str();
        }
        connection::error err() const NOTHROW
        {
            return error_;
        }
    private:
        const std::string what_;
        const connection::error error_;
    };

    connection::error exception_to_errorcode()
    {
        auto err = connection::error::unknown;
        try
        {
            throw;
        }
        catch (const conn_exception& e)
        {
            err = e.err();
        }
        catch (const protocol::exception& e)
        {
            const auto code = e.code();
            if (code == protocol::error::authentication_failed)
                err = connection::error::forbidden;
            else err = connection::error::protocol;
        }
        catch (const socket::tcp_exception& e)
        {
            const auto code = e.code();
            if (code == std::errc::connection_refused)
                err = connection::error::refused;
            else err = connection::error::socket;
        }
        catch (const socket::ssl_exception& e)
        {
            err = connection::error::ssl;
        }
        catch (const std::exception& e)
        {
            err = connection::error::unknown;
        }
        return err;
    }

    const char* errcode_to_string(connection::error error)
    {
    #define CASE(x) case x: return #x;

        switch (error)
        {
            CASE(connection::error::none);
            CASE(connection::error::resolve);
            CASE(connection::error::refused);
            CASE(connection::error::forbidden);
            CASE(connection::error::protocol);
            CASE(connection::error::socket);
            CASE(connection::error::ssl);
            CASE(connection::error::timeout);
            CASE(connection::error::interrupted);
            CASE(connection::error::unknown);
        }
    #undef CASE

        assert(!"missing error case");
        return "???";
    }

} // namespace

namespace newsflash
{

struct connection::thread_data {
    std::unique_ptr<socket> sock;
    std::string   logfile;
    std::string   host;
    std::uint16_t port;
    bool          ssl;
};

connection::connection(std::string logfile) : logfile_(std::move(logfile))
{}

connection::~connection()
{
    shutdown_.set();
    cancel_.set();
    thread_->join();
}

void connection::connect(const std::string& host, std::uint16_t port, bool ssl)
{
    assert(!thread_);

    std::unique_ptr<connection::thread_data> data(new connection::thread_data);
    data->host = host;
    data->port = port;
    data->ssl  = ssl;

    thread_.reset(new std::thread(std::bind(&connection::main, this, data.get())));
    data.release();
}

void connection::execute(std::shared_ptr<cmdlist> cmds)
{
    std::lock_guard<std::mutex> lock(mutex_);
    cmds_ = cmds;
    execute_.set();
}

void connection::cancel()
{
    cancel_.set();
}

void connection::main(thread_data* data)
{
    std::unique_ptr<thread_data> unique(data);

    LOG_OPEN(logfile_);
    LOG_D("Connection thread ", std::this_thread::get_id());

    try
    {
        LOG_I("Connecting to '", data->host, ":", data->port, "'");

        connect(data);

        LOG_I("Connection ready");

        // init protocol stack
        protocol proto;
        proto.on_log  = [](const std::string& str) { LOG_I(str); };
        proto.on_auth = on_auth;
        proto.on_send = std::bind(&connection::send, this, data,
            std::placeholders::_1, std::placeholders::_2);
        proto.on_recv = std::bind(&connection::recv, this, data,
            std::placeholders::_1, std::placeholders::_2);    

        // perfrorm handshake
        proto.connect();

        const std::chrono::seconds PING_INTERVAL(10);

        for (;;)
        {
            on_ready();

            auto execute  = execute_.wait();
            auto shutdown = shutdown_.wait();
            if (!newsflash::wait_for(execute, shutdown, PING_INTERVAL))
                proto.ping();
            else if (execute)
            {
                std::unique_lock<std::mutex> lock(mutex_);
                std::shared_ptr<cmdlist> cmds = cmds_;
                cmds_.reset();
                execute_.reset();
                lock.unlock();

                while (cmds->run(proto))
                {
                    if (is_set(cancel_))
                    {
                        cancel_.reset();
                        break;
                    }
                }
            }
            else if (shutdown)
                break;
        }
    }
    catch (const std::exception& e)
    {
        const auto code = exception_to_errorcode();
        const auto estr = errcode_to_string(code);        
        on_error(code);
        LOG_E("Connection error '", estr, "' ", e.what());
    }

    LOG_D("Thread exiting");
    LOG_CLOSE();
}

void connection::connect(thread_data* data)
{
    const auto addr = resolve_host_ipv4(data->host);
    if (!addr)
        throw conn_exception("failed to resolve host", error::resolve);

    LOG_I("Resolved to '", format_ipv4(addr), "'");

    if (data->ssl)
        data->sock.reset(new newsflash::sslsocket());
    else data->sock.reset(new newsflash::tcpsocket());

    data->sock->begin_connect(addr, data->port);

    auto connect  = data->sock->wait(true, false);
    auto shutdown = shutdown_.wait();
    newsflash::wait_for(connect, shutdown);

    if (shutdown)
        throw conn_exception("connection was interrupted", error::interrupted);

    data->sock->complete_connect();
}

void connection::send(thread_data* data, const void* buff, std::size_t len)
{
    // the commands that we're sending are very small
    // so presumably there's no problem here.
    data->sock->sendall(buff, (int)len);

}

size_t connection::recv(thread_data* data, void* buff, std::size_t len)
{
    // if there's no data in timeout seconds we consider
    // the connection stale and timeout.
    // also note that the protocol stack excepts this 
    // function to always return the data (or throw) so 
    // we must then throw on timeout.

    const std::chrono::seconds TIMEOUT(15);    

    auto canread  = data->sock->wait(true, false);
    auto shutdown = shutdown_.wait();

    if (!newsflash::wait_for(canread, shutdown, TIMEOUT))
        throw conn_exception("socket timeout", error::timeout);
    else if (shutdown)
        throw conn_exception("read interrupted", error::interrupted);

    // todo: throttle
    // todo: handle 0 recv when shutting down.
    const size_t ret = data->sock->recvsome(buff, (int)len);

    if (on_read)
        on_read(ret);

    return ret;
}


} // newsflash
