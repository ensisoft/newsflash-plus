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

namespace newsflash
{
    class cmdlist;
    class action;
    class session;
    class socket;

    class connection
    {
    public:
        enum class error {
            none, 
            authentication_rejected,
            no_permission
        };

        enum class state {
            disconnected, 
            resolving, 
            connecting,
            initialize, 
            connected, 
            active, 
            disconnecting,
            error
        };

        class exception : public std::exception
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


        std::function<void(std::unique_ptr<action>)> on_action;

        struct server {
            std::string username;
            std::string password;
            std::string hostname;
            std::uint16_t port;
            bool use_ssl;
        };

        connection();
       ~connection();

        void connect(const connection::server& serv);
        
        void disconnect();

        void execute(cmdlist* cmds);

        bool complete(std::unique_ptr<action> act);

        state get_state() const 
        { return state_; }

    private:
        class resolve;
        class connect;
        class initialize;
        class execute;

    private:
        std::unique_ptr<socket> socket_;
        std::unique_ptr<session> session_;
    private:
        state state_;
    private:
        server serv_;
    };

} // newsflash


