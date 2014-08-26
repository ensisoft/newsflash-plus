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

    class session
    {
    public:
        enum class error {
            none, 
            service_temporarily_unavailable,
            service_permanently_unavailable,
            authentication_rejected,
            no_permission
        };

        enum class state {
            none, init, authenticate, ready, error
        };

        // called when session wants to send data
        std::function<void (const std::string& cmd)> on_send;

        // called when session wants to authenticate
        std::function<void (std::string& user, std::string& pass)> on_auth;

        session();
       ~session();

        void reset();

        bool initialize(buffer& buff);

        error get_error() const;

        state get_state() const;

        bool has_xzver() const;

        bool has_gzip_compress() const;

    private:
        struct impl;

        class command;
        class welcome;
        class getcaps;
        class modereader;
        class authuser;
        class authpass;

    private:
        std::unique_ptr<command> current_;
        std::unique_ptr<command> retry_;
        std::unique_ptr<impl> state_;
    };

} // newsflash