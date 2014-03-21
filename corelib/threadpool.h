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

#include <condition_variable>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <cstddef>

namespace corelib
{
    class threadpool
    {
    public:
        class work 
        {
        public:
            virtual ~work() = default;

            // execute this chunk of work
            virtual void execute() = 0;
        protected:
        private:
        };

        // thread identifier type
        typedef std::size_t tid_t;

        // initialize the pool with num_threads.
        // precondition: num_threads > 0
        threadpool(std::size_t num_threads);
       ~threadpool();

        // allocate a thread from the pool.
        // the thread is subsequently identified by the returned tid.
        tid_t allocate();

        // submit work to the threadpool to be executed the thread
        // specified by the tid.
        // the ownership of the work object to be executed
        // is transferred to the threadpool.
        void submit(std::unique_ptr<work> work, tid_t key);

        // submit work to the threadpool with the given
        // thread affinity key.
        // the ownership of the work object remains with the caller
        // and the object needs to remain valid untill the work
        // has been executed.
        void submit(work* work, tid_t key);

        // block untill all the work queues are drained.
        void drain();

   private:
        enum class action {
            none, quit, work
        };

        struct thread {
            std::mutex mutex;
            std::condition_variable cond;
            std::unique_ptr<std::thread> thread;
            std::queue<std::unique_ptr<work>> queue;
            action act;
        };

        typedef std::vector<std::unique_ptr<thread>> container;

    private:
        void thread_main(threadpool::thread* self);

    private:
        std::condition_variable cond_;
        std::mutex mutex_;
        std::size_t qsize_;
        container threads_;
        tid_t key_;
    };

} // corelib
