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

#include <condition_variable>
#include <memory>
#include <thread>
#include <mutex>
#include <string>
#include <queue>

namespace newsflash
{
    class cmdlist;

    class connection
    {
    public:
        enum class error {
            none, 
            resolve_host,
            connection_refused, 
            authentication_failed,
            forbidden,
            service_temporarily_unavailable,
            service_permanently_unavailable,
            timeout,
            tcp, ssl, nntp,
            other
        };

        enum class state {
            disconnected, 
            connecting, 
            ready, active, error
        };

        connection(std::string logfile);
       ~connection();

        void shutdown();

        void connect(std::string username, std::string password, 
            std::string hostname, std::uint16_t port, bool ssl);

        void execute(cmdlist* cmds);

        void cancel();

    private:
        void thread_loop(std::string logfile);

    private:
        class exception;
    private:
        class action;
        class connect;
        class execute;
        class shutdown;

    private:
        std::unique_ptr<std::thread> thread_;
        std::queue<std::unique_ptr<action>> actions_;
        std::mutex mutex_;
        std::condition_variable cond_;
    };

} // newsflash


