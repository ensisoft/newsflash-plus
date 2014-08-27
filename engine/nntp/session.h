// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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
#include <deque>
#include <memory>
#include "response.h"

namespace nntp
{
    class session
    {
    public:
        enum class error {
            none,
            authentication_failed,
            service_temporarily_unavailable,
            service_permanently_unavailable
        };

        enum class status {
            none, error, want_read, want_write
        };

        std::function<size_t (void* buff, size_t len)> on_recv;

        // send data callback
        std::function<void (const void* buff, std::size_t len)> on_send;

        // authentication callback
        std::function<void (std::string& user, std::string& pass)> on_auth;

        session();

       ~session();

        status check();

        error err() const;

        void start();

        void write();

        void read();

    private:
        class command;
        class state;
        class welcome;
        class capabilities;        
        class modereader;
        class authenticate;
        class pipeline;

    private:
        std::unique_ptr<state> state_;
        std::deque<std::unique_ptr<command>> send_;
        std::deque<std::unique_ptr<command>> recv_;
        std::string current_group_;
        bool has_compress_gzip_;
        bool has_xzver_;
        bool has_mode_reader_;
        response buff_;
    };

} // nntp
