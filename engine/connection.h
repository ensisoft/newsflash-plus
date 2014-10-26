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

#include <system_error>
#include <stdexcept>
#include <string>
#include <memory>
#include "action.h"

namespace newsflash
{
    class cmdlist;

    class connection
    {
        struct state;
    public:
        enum class error {
            resolve,
            authentication_rejected,
            no_permission,
            timeout,
            network
        };
        class exception : public std::exception
        {
        public:
            exception(connection::error err, std::string what) 
                : error_(err), what_(std::move(what))
            {}

            const char* what() const noexcept
            { return what_.c_str(); }

            connection::error error() const noexcept
            { return error_; }

        private:
            connection::error error_;
            std::string what_;
        };

        // perform host resolution
        class resolve : public action
        {
        public:
            resolve(std::shared_ptr<state> s) : state_(s)
            {}

            virtual void xperform() override;
        private:
            std::shared_ptr<state> state_;
        };

        // perform socket connect 
        class connect : public action
        {
        public:
            connect(std::shared_ptr<state> s);

            virtual void xperform() override;
        private:
            std::shared_ptr<state> state_;
        };

        // perform nntp init
        class initialize : public action
        {
        public:
            initialize(std::shared_ptr<state> s);

            virtual void xperform() override;
        private:
            std::shared_ptr<state> state_;
        };

        // execute cmdlist 
        class execute : public action
        {
        public:
            execute(std::shared_ptr<state> s);

            virtual void xperform() override;
        private:
            std::shared_ptr<state> state_;
        };

        class disconnect : public action
        {
        public:
            disconnect(std::shared_ptr<state> s);

            virtual void xperform() override;
        private:
            std::shared_ptr<state> state_;
        };

        class ping : public action
        {
        public:
            ping(std::shared_ptr<state> s);

            virtual void xperform() override;

        private:
            std::shared_ptr<state> state_;
        };

        struct spec {
            std::string username;
            std::string password;
            std::string hostname;
            std::uint16_t hostport;         
            bool use_ssl;
        };

        // begin connecting to the given host specification.
        std::unique_ptr<action> connect(spec s);

        // perform disconnect from the host. 
        std::unique_ptr<action> disconnect();

        // perform ping
        std::unique_ptr<action> ping();

        // complete the given action. returns a new action if any.
        std::unique_ptr<action> complete(std::unique_ptr<action> a);

        // execute the given cmdlist
        std::unique_ptr<action> execute(std::shared_ptr<cmdlist> cmd);

        void cancel();

    private:
        std::shared_ptr<state> state_;

    };

    


} // newsflash


