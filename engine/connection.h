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

#pragma once

#include <newsflash/config.h>

#include <functional>
#include <memory>
#include <string>
#include <stdexcept>
#include <cstdint>
#include "ui/connection.h"
#include "event.h"

namespace newsflash
{
    class cmdlist;
    class action;
    class session;
    class socket;

    class connection
    {
    public:
        using error = ui::connection::error;
        using state = ui::connection::state;

        std::function<void(std::unique_ptr<action>)> on_action;

        struct server {
            std::string username;
            std::string password;
            std::string hostname;
            std::uint16_t port;
            bool use_ssl;
        };

        connection(std::size_t account, std::size_t id);
       ~connection();

        void connect(const connection::server& serv);
        
        void disconnect();

        void execute(cmdlist* cmds);

        void complete(std::unique_ptr<action> act);

        void cancel();

        state get_state() const 
        { return ui_.st; }

        error get_error() const 
        { return ui_.err; }

        std::size_t get_account() const
        { return ui_.account; }

        std::size_t get_id() const 
        { return ui_.id; }

        ui::connection get_ui_state() const
        { return ui_; }

    private:
        class exception;
        class resolve;
        class connect;
        class initialize;
        class execute;

    private:
        std::unique_ptr<socket> socket_;
        std::unique_ptr<session> session_;
    private:
        event cancellation_;        
    private:
        server serv_;
    private:
        ui::connection ui_;
    };

} // newsflash


