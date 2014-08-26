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
#include "command.h"
#include "buffer.h"
#include "session.h"
#include "nntp.h"

namespace newsflash
{
struct connection::data {
    newsflash::session session;
    newsflash::buffer buffer;
    connection::error error;
    std::unique_ptr<newsflash::socket> socket;
    std::queue<command*> pipeline;
};

class connection::state
{
public:
    enum class type {
        connect, initialize, transfer
    };

    virtual ~state() = default;

    virtual bool readsocket(data& data) = 0;

    virtual type identity() const = 0;
protected:
private:
};

class connection::connect : public state
{
public:
    virtual bool readsocket(data& d) override
    {
        d.socket->complete_connect();
        return true;
    }
    virtual type identity() const override
    { return type::connect; }
private:
};

class connection::initialize : public state
{
public:
    virtual bool readsocket(data& d) override
    {
        auto& buffer  = d.buffer;
        auto& session = d.session;
        const auto cursize = buffer.size();
        const auto ret = d.socket->recvsome(buffer.back(), buffer.available());
        buffer.resize(cursize + ret);
        if (session.initialize(buffer))
        {
            buffer.clear();
            switch (session.get_error())
            {
                case session::error::none:
                    break;
                case session::error::service_temporarily_unavailable:
                    break;
                case session::error::service_permanently_unavailable:
                    break;
                case session::error::authentication_rejected:
                    break;
                case session::error::no_permission:
                    break;
            }
            //for (auto* cmd : d.pipeline)
            //    cmd->start(session);
            return true;
        }
        return false;
    }
    virtual type identity() const override 
    { return type::initialize; }
private:
};

class connection::transfer : public state
{
public:
    virtual bool readsocket(data& d) override
    {
        auto& buff  = d.buffer;
        auto& session = d.session;
        const auto cursize = buff.size();
        const auto bytes = d.socket->recvsome(buff.back(), buff.available());
        buff.resize(cursize + bytes);

        auto* command = d.pipeline.front();
        buffer body;
        if (command->process(session, buff, body))
        {
            //const auto err = command->e
            return true;
        }
        return false;
    }
    virtual type identity() const override
    { return type::transfer; }
private:
};


connection::connection() : data_(new data)
{
    data_->session.on_send = std::bind(&connection::do_send, this,
        std::placeholders::_1);
    data_->session.on_auth = std::bind(&connection::do_auth, this, 
        std::placeholders::_1, std::placeholders::_2);
}

connection::~connection() 
{}

void connection::connect(ipv4addr_t host, port_t port, bool ssl)
{
    if (ssl)
         data_->socket.reset(new sslsocket());
    else data_->socket.reset(new  tcpsocket());
    data_->socket->begin_connect(host, port);

    state_.reset(new class connect);
    data_->buffer.clear();
    data_->session.reset();

    LOG_I("Connecting to _1:_2 SSL: _3", format_ipv4(host), port, ssl);
}


void connection::enqueue(command* cmd)
{
    //cmd->start(session_);
    //pipeline_.push(cmd);
}


bool connection::readsocket()
{
    try
    {

        if (state_->readsocket(*data_))
            return false;

        switch (state_->identity())
        {
            case state::type::connect:
                state_.reset(new initialize);
                break;
            case state::type::initialize:
                state_.reset(new transfer);
                break;

            case state::type::transfer:
                break;
        }
    }
    catch (const socket::tcp_exception& e)
    {}
    catch (const socket::ssl_exception& e)
    {
    }
    catch (const nntp::exception& e)
    {}
    catch (const std::exception& e)
    {}
    return true;
}

void connection::do_send(const std::string& cmd)
{
    if (cmd.find("AUTHINFO PASS") != std::string::npos)
        LOG_I("AUTHINFO PASS *******");
    else LOG_I(cmd);

    data_->socket->sendall(cmd.data(), cmd.size());
}

} // newsflash
