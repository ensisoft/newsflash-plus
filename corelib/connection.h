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

#include <boost/noncopyable.hpp>
#include <condition_variable>
#include <system_error>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <cstdint>
#include "event.h"

namespace corelib
{
    class cmdlist;

    // connection to a newsserver
    class connection : boost::noncopyable
    {
    public:
        enum class error {
            resolve,     // failed to resolve host address
            refused,     // host refused the connection attempt
            forbidden,   // connection was made but user credentials were refused
            protocol,    // nntp protocol level error occurred
            socket,      // tcp socket error occurred
            ssl,         // ssl error occurred
            timeout,     // no data was received from host within timeout
            interrupted, // pending operation was interrupted 
            unknown      // something else.
        };

        // invoked when connection is ready to execute a cmdlist
        std::function<void ()> on_ready;

        // invoked when connection encounters an error.
        // provides a higher level connection error and a lower level
        // system error code. system error code can be no-error when
        // for example we have a protocol error.
        std::function<void (connection::error error, const std::error_code& code)> on_error;

        // invoked when data is read
        std::function<void (std::size_t bytes)> on_read;

        // invoked when authentication data is needed
        std::function<void (std::string& user, std::string& pass)> on_auth;

        connection();
       ~connection();

        // start the connection
        void connect(const std::string& logfile, const std::string& host, std::uint16_t port, bool ssl);

        // execute the commands in the given cmdlist.
        // the cmdlist is run() repeatedly untill the cmdlist is
        // complete or the cmdlist is canceled via a call to cancel()
        void execute(cmdlist* cmds);

        // cancel the current cmdlist.
        void cancel();

    private:
        struct thread_data;

        void thread_main(thread_data* data);
        void thread_connect(thread_data* data);
        void thread_send(thread_data* data, const void* buff, std::size_t len);
        size_t thread_recv(thread_data* data, void* buff, std::size_t len);        

    private:
        std::unique_ptr<std::thread> thread_;
        std::mutex mutex_;
        event shutdown_;
        event execute_;
        event cancel_;
        cmdlist* cmds_;
    };

} // corelib


