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
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <cstddef>

namespace newsflash
{
    class action;

    class threadpool
    {
    public:
        std::function<void (action*)> on_complete;

        // initialize the pool with num_threads.
        // precondition: num_threads > 0
        threadpool(std::size_t num_threads);
       ~threadpool();

        void submit(action* act);

        void wait();

        void shutdown();

        std::size_t num_pending_actions() const
        { return queue_size_; }

   private:
        struct thread {
            std::mutex mutex;
            std::condition_variable cond;
            std::unique_ptr<std::thread> thread;
            std::queue<action*> queue;
            bool run_loop;
        };

        typedef std::vector<std::unique_ptr<thread>> container;

    private:
        void thread_main(threadpool::thread* self);

    private:
        std::condition_variable cond_;
        std::mutex mutex_;
        std::vector<std::unique_ptr<thread>> threads_;
        std::size_t round_robin_;
        std::atomic<std::size_t> queue_size_;
    };

} // newsflash
