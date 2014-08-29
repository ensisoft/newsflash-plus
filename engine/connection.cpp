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

namespace newsflash
{

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

// resolve the host name to an ipv4 address 
class connection::resolve : public action
{
public:
    resolve(std::string hostname) : hostname_(std::move(hostname)), address_(0)
    {}

    virtual void xperform() override
    {
        address_ = resolve_host_ipv4(hostname_);
        if (address_ == 0)
        {
            std::error_code errc = get_last_socket_error();
            throw std::system_error(errc, "resolve host failed");
        }
    }

    ipv4addr_t get_resolved_address() const 
    { return address_; }

private:
    std::string hostname_;
private:
    ipv4addr_t address_;
};

// try to connect a socket to the given resolved host address and port
class connection::connect : public action
{
public:
    connect(event& cancel, ipv4addr_t addr, std::uint16_t port, bool ssl) : addr_(addr), port_(port), cancel_(cancel)
    {
        if (ssl)
            socket_.reset(new sslsocket);
        else socket_.reset(new tcpsocket);
    }

    virtual void xperform() override
    {
        socket_->begin_connect(addr_, port_);

        auto connect = socket_->wait(true, false);
        auto cancel  = cancel_.wait();
        newsflash::wait(connect, cancel);
        if (cancel)
            return;

        socket_->complete_connect();
    }
    std::unique_ptr<socket> get_connected_socket() 
    { return std::move(socket_); }

private:
    std::unique_ptr<socket> socket_;
private:    
    ipv4addr_t addr_;
    std::uint16_t port_;
private: 
    event& cancel_;
};

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
        session_->start();

        newsflash::buffer buff(1024);
        newsflash::buffer temp(1);
        while (session_->pending())
        {
            while (!session_->parse_next(buff, temp))
            {
                auto data   = socket_->wait(true, false);
                auto cancel = cancel_.wait();
                newsflash::wait(data, cancel);
                if (cancel) 
                    return;

                const auto bytes = socket_->recvsome(buff.back(), buff.available());
                if (bytes == 0) 
                    throw std::runtime_error("socket was closed  unexpectedly");
                buff.append(bytes);
            }
        }
        const auto err = session_->get_error();
        if (err == session::error::authentication_rejected)
            throw connection::exception("authentication rejected", connection::error::authentication_rejected);
        else if (err == session::error::no_permission)
            throw connection::exception("no permission", connection::error::no_permission);
    }
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

connection::connection(std::size_t account, std::size_t id)
{
    ui_.err     = error::none;
    ui_.account = account;
    ui_.id      = id;
    ui_.task    = 0;
    ui_.down    = 0;
    ui_.host    = "";
    ui_.desc    = "";
    ui_.secure  = false;
    ui_.bps     = 0;
}

connection::~connection()
{}

void connection::connect(const connection::server& serv)
{
    cancellation_.reset();

    std::unique_ptr<newsflash::action> act(new connection::resolve(serv.hostname));
    on_action(std::move(act));
    serv_  = serv;
    ui_.st     = state::resolving;
    ui_.err    = error::none;    
    ui_.host   = serv.hostname;
    ui_.secure = serv.use_ssl;
}

void connection::complete(std::unique_ptr<action> act)
{
    if (act->has_exception())
    {
        socket_.reset();
        session_.reset();
        ui_.st = state::error;
        try
        {
            act->rethrow();
        }
        catch (const exception& e)
        {
            LOG_E(e.what());
            ui_.err = e.error();
        }
        catch (const socket::tcp_exception& e)
        {
            const auto errc = e.code();
            LOG_E(e.what(), ", ", errc.message());            
            ui_.err = error::network;
        }
        catch (const std::system_error& e)
        {
            LOG_E("System error: ", e.what(), e.code().message());
            
        }
        catch (const std::exception& e)
        {
            LOG_E("Exception: ", e.what());
            ui_.err = error::other;
        }
        return;
    }

    std::unique_ptr<action> next;

    switch (ui_.st)
    {
        case state::resolving:
            {
                const auto& action_resolve_host = dynamic_cast<resolve&>(*act);
                const auto resolved_address = action_resolve_host.get_resolved_address();
                const auto port = serv_.port;
                const auto ssl = serv_.use_ssl;
                next.reset(new class connect(cancellation_, resolved_address, port, ssl));
                ui_.st = state::connecting;
            }
            break;

        case state::connecting:
            {
                auto& action_connect = dynamic_cast<class connect&>(*act);
                const auto username = serv_.username;
                const auto password = serv_.password;
                auto connected_socket = action_connect.get_connected_socket();
                next.reset(new class initialize(cancellation_, std::move(connected_socket), username, password));
                ui_.st = state::initializing;
            }
            break;

        case state::initializing:
            {

                ui_.st = state::connected;
            }
            break;

        case state::active:
            {

                ui_.st = state::connected;
            }
            break;

        case state::disconnecting:
            {
                ui_.st = state::disconnected;
            }
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

} // newsflash
