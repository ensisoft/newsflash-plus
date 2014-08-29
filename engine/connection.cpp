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

// resolve the host name to an ipv4 address 
class connection::resolve : public action
{
public:
    resolve(std::string hostname) : hostname_(std::move(hostname)), address_(0)
    {}

    virtual void perform(event& cancellation) override
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
    connect(ipv4addr_t addr, std::uint16_t port, bool ssl) : addr_(addr), port_(port)
    {
        if (ssl)
            socket_.reset(new sslsocket);
        else socket_.reset(new tcpsocket);
    }

    virtual void perform(event& cancellation) override
    {
        socket_->begin_connect(addr_, port_);

        auto connect = socket_->wait(true, false);
        auto cancel  = cancellation.wait();
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
};

class connection::initialize : public action
{
public:
    initialize(std::unique_ptr<socket> socket, std::string username, std::string password) 
       : socket_(std::move(socket)), username_(std::move(username)), password_(std::move(password))
    {
        session_.reset(new session);
        session_->on_auth = std::bind(&initialize::do_auth, this, 
            std::placeholders::_1, std::placeholders::_2);
        session_->on_send = std::bind(&initialize::do_send, this,
            std::placeholders::_1);
    }
    virtual void perform(event& cancellation) override
    {
        session_->start();

        newsflash::buffer buff(1024);
        newsflash::buffer temp(1);
        while (session_->pending())
        {
            auto data   = socket_->wait(true, false);
            auto cancel = cancellation.wait();
            newsflash::wait(data, cancel);
            if (cancel) 
                return;

            while (!session_->parse_next(buff, temp))
            {
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

};

connection::connection() : state_(state::disconnected)
{}

connection::~connection()
{}

void connection::connect(const connection::server& serv)
{
    std::unique_ptr<newsflash::action> act(new connection::resolve(serv.hostname));
    on_action(std::move(act));
    serv_  = serv;
    state_ = state::resolving;
}

bool connection::complete(std::unique_ptr<action> act)
{
    if (act->has_exception())
    {
        socket_.reset();
        session_.reset();
        state_ = state::error;

        act->rethrow();
    }

    std::unique_ptr<action> next;

    switch (state_)
    {
        case state::resolving:
            {
                const auto& action_resolve_host = dynamic_cast<resolve&>(*act);
                const auto resolved_address = action_resolve_host.get_resolved_address();
                const auto port = serv_.port;
                const auto ssl = serv_.use_ssl;
                next.reset(new class connect(resolved_address, port, ssl));
                state_ = state::connecting;
            }
            break;

        case state::connecting:
            {
                auto& action_connect = dynamic_cast<class connect&>(*act);
                const auto username = serv_.username;
                const auto password = serv_.password;
                auto connected_socket = action_connect.get_connected_socket();
                next.reset(new class initialize(std::move(connected_socket), username, password));
                state_ = state::initialize;
            }
            break;

        case state::initialize:
            {

                state_ = state::connected;
            }
            break;

        case state::active:
            {

                state_ = state::connected;
            }
            break;

        case state::disconnecting:
            {
                state_ = state::disconnected;
            }
            break;

        case state::disconnected:
        case state::connected:
        case state::error:
            assert(!"wtufh");
            break;
    }

    if (!next)
        return true;

    on_action(std::move(next));
    return false;

}

} // newsflash
