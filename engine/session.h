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
#include <string>
#include <memory>
#include <cstddef>

namespace newsflash
{
    class buffer;

    // implements NNTP session state
    class session
    {
    public:
        enum class error {
            none, 
            authentication_failed,
            service_temporarily_unavailable,
            service_permanently_unavailable
        };

        // called when session wants to send data
        std::function<void (const std::string& cmd)> on_send;

        // called for logging events
        std::function<void (const std::string& str)> on_log;

        // called when session wants to authenticate
        std::function<void (std::string& user, std::string& pass)> on_auth;

        session();
       ~session();

        // this should be called when data link connection
        // is established and data begins to flow.
        void start();

        // call inspect to inspect the contents of the buffer and 
        // change the session state when complete command responses
        // are parsed out of the buffer.
        // returns true if a complete message was found, otherwise false
        // in which case further data should be received into the buffer.
        bool inspect(buffer& buff);

        struct capabilities {
            bool gzip; // gzip compression
            bool xzver; // compressed headers
            bool modereader;
        };

    private:
        struct state;

        class command;
        class welcome;
        class getcaps;
        class modereader;
        class authuser;
        class authpass;

    private:
        std::unique_ptr<command> command_;
        std::unique_ptr<command> needs_authentication_;
        std::unique_ptr<state> state_;
    };

} // newsflash