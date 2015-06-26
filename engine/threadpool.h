// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
// 
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <newsflash/config.h>

#include <condition_variable>
#include <functional>
#include <memory>
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
        struct worker;

        // callback to be invoked when an action has been completed
        std::function<void (action*)> on_complete;

        // initialize the pool with num_threads.
        // precondition: num_threads > 0
        threadpool(std::size_t num_threads);
       ~threadpool();

        // submit an action to the threadpool for any thread to execute specific
        // the thread affinity constraint set on the action.
        void submit(action* act);

        // submit work to the specific thread
        void submit(action* act, worker* t);

        // wait for all actions to be completed before returning.
        void wait_all_actions();

        // shutdown the threadpool. will block and join all the threads.
        void shutdown();

        // allocate a private thread.
        // returns a handle to the private thread.
        worker* allocate();

        // detach and return the thread to the threadpool.
        // the thread will continue to perform all the actions queued
        // in it's queue and then become eligible for using again
        void detach(worker* t);

        std::size_t num_pending_actions() const
        { return queue_size_; }

   private:
        void thread_main(threadpool::worker* self);

    private:
        std::condition_variable cond_;
        std::mutex mutex_;
        std::vector<std::unique_ptr<worker>> threads_;
        std::size_t round_robin_;
        std::size_t pool_size_;
        std::atomic<std::size_t> queue_size_;
    };

} // newsflash
