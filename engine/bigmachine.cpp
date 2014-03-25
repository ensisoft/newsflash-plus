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

#include <corelib/connection.h>
#include <corelib/taskstate.h>
#include <corelib/threadpool.h>
#include <corelib/download.h>
#include <corelib/logging.h>
#include <corelib/assert.h>
#include <corelib/filesys.h>
#include <corelib/queue.h>
#include <vector>
#include <thread>
#include "configuration.h"
#include "listener.h"
#include "account.h"
#include "server.h"
#include "engine.h"

namespace {
    struct message_handler {
        virtual ~message_handler() = default;
    };

    struct message {
        virtual ~message() = default;
    };

} //

namespace engine
{

class engine::impl : public message_handler
{
public:
    impl(listener& callback, std::string logs) : logs_(std::move(logs)), listener_(callback)
    {
        bytes_downloaded_ = 0;
        bytes_written_    = 0;
        bytes_queued_     = 0;
        thread_.reset(new std::thread(std::bind(&engine::impl::thread_main, this)));
    }

   ~impl()
    {}

private:
    void thread_main()
    {
        LOG_OPEN(fs::joinpath(logs_, "engine.log"));
        LOG_D("Engine thread: ", std::this_thread::get_id());

        for (;;)
        {

        }

        LOG_D("Engine thread exiting...");
        LOG_CLOSE();
    }

private:
    const std::string logs_;

private:
    std::vector<account> accounts_;
    std::unique_ptr<std::thread> thread_;
    std::uint64_t bytes_downloaded_;
    std::uint64_t bytes_written_;
    std::uint64_t bytes_queued_;
    listener& listener_;
    configuration config_;

};


engine::engine(listener& callback, std::string logs) : pimpl_(new impl(callback, std::move(logs)))
{}

void engine::configure(const configuration& conf)
{

}
void engine::add(const account& acc)
{

}


} // warez
