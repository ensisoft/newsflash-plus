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

#include <functional>
#include <cassert>
#include "threadpool.h"

namespace newsflash
{

threadpool::threadpool(std::size_t num_threads) :qsize_(0), key_(0)
{
    for (std::size_t i=0; i<num_threads; ++i)
    {
        std::unique_ptr<threadpool::thread> thread(new threadpool::thread);
        thread->act = action::none;
        thread->thread.reset(new std::thread(
            std::bind(&threadpool::thread_main, this, thread.get())));
        threads_.push_back(std::move(thread));
    }
}

threadpool::~threadpool()
{
    for (auto& thread : threads_)
    {
        std::lock_guard<std::mutex> lock(thread->mutex);
        thread->act = action::quit;
        thread->cond.notify_one();
        thread->thread->join();
    }
}

threadpool::key_t threadpool::allocate()
{
    key_t ret = key_ % threads_.size();
    ++key_;
    return ret;
}

void threadpool::submit(std::unique_ptr<work> work, key_t key)
{
    assert(key < threads_.size());
    auto& thread = threads_[key];

    std::lock_guard<std::mutex> lock(thread->mutex);

    thread->queue.push(std::move(work));
    thread->act = action::work;
    thread->cond.notify_one();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++qsize_;
    }
}

void threadpool::drain()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock (mutex_);
        if (!qsize_)
            return;
        cond_.wait(lock);
    }
}

void threadpool::thread_main(threadpool::thread* self)
{
    for (;;)
    {
        std::unique_ptr<work> work;
        action act = action::none;
        do 
        {
            std::unique_lock<std::mutex> lock(self->mutex);
            if (self->act == action::none)
                self->cond.wait(lock);

            if (self->act == action::work)
            {
                assert(!self->queue.empty());
                work = std::move(self->queue.front());
                self->queue.pop();
                if (self->queue.empty())
                    self->act = action::none;

                act = action::work;
            }
            else if (self->act == action::quit)
                act = action::quit;
        }
        while (act == action::none);

        if (act == action::quit)
            break;

        work->execute();

        // support drain signaling
        std::lock_guard<std::mutex> lock(mutex_);
        if (--qsize_ == 0)
            cond_.notify_one();
    }
}

} // newsflash
