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
#include "ui/error.h"
#include "event.h"

namespace newsflash
{
    class cmdlist;
    class action;
    class session;
    class socket;

    // connection class handles the details of a NNTP connection
    // including buffering, reading and writing from/to socket and
    // nntp session management. 
    // each connection method such as connecting to a remote host
    // is split into multiple actions. Each action then represents a single
    // step in the whole operation. each action is invoked through the
    // on_action callback. once the action has been performed the 
    // complete(action) function should be called to complete
    // the pending state transition. after a call to complete
    // the connection state should be inspected for possible error condition.
    class connection
    {
    public:
        using error = ui::connection::error;
        using state = ui::connection::state;

        // this callback is invoked when theres a new action to be performed.
        std::function<void(std::unique_ptr<action>)> on_action;

        // this callback is invoked when an error occurs.
        // the error object carries more error information.
        std::function<void(const ui::error&)> on_error;

        struct server {
            std::string username;
            std::string password;
            std::string hostname;
            std::uint16_t port;
            bool use_ssl;
        };

        // create a new connection with the given account and connection ids.
        connection(std::size_t account, std::size_t id);
       ~connection();

        // begin connecting to the given server.
        void connect(const connection::server& serv);
        
        // begin disconnect
        void disconnect();

        // execute the given cmdlist. also update the current
        // connection status to the given descriptor and task ids.
        void execute(std::string desc, std::size_t task, cmdlist* cmds);

        // complete the given action and complete the pending state transition.
        void complete(std::unique_ptr<action> act);

        // cancel the pending action.
        void cancel();

        // observers
        state get_state() const 
        { return ui_.st; }

        error get_error() const 
        { return ui_.err; }

        std::size_t get_account() const
        { return ui_.account; }

        std::size_t get_id() const 
        { return ui_.id; }

        // get all state visible towards the UI
        ui::connection get_ui_state() const
        { return ui_; }

    private:
        class exception;
        class resolve;
        class connect;
        class initialize;
        class execute;
        class disconnect;

    private:
        std::unique_ptr<socket> socket_;
        std::unique_ptr<session> session_;
        std::string username_;
        std::string password_;
        std::uint16_t port_;
        bool ssl_;
    private:
        event cancellation_;        
    private:
        ui::connection ui_;
    };

} // newsflash


