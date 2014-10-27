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
#include "action.h"

namespace newsflash
{

threadpool::threadpool(std::size_t num_threads) : round_robin_(0), queue_size_(0)
{
    assert(num_threads);

    for (std::size_t i=0; i<num_threads; ++i)
    {
        std::unique_ptr<threadpool::thread> thread(new threadpool::thread);
        std::lock_guard<std::mutex> lock(thread->mutex);

        thread->run_loop = true;
        thread->thread.reset(new std::thread(std::bind(&threadpool::thread_main, this, thread.get())));
        threads_.push_back(std::move(thread));
    }
}

threadpool::~threadpool()
{

}

void threadpool::submit(action* act)
{
    const auto num_threads  = threads_.size();
    const auto affinity = act->get_affinity();
    thread* worker = nullptr;

    if (affinity == action::affinity::any_thread)
    {
        worker = threads_[round_robin_ % num_threads].get();
        round_robin_++;
    }
    else
    {
        const auto act_id = act->get_id();        
        worker = threads_[act_id % num_threads].get();
    }

    std::lock_guard<std::mutex> lock(worker->mutex);
    worker->queue.push(act);
    worker->cond.notify_one();    

    queue_size_++;    
}

void threadpool::wait()
{
    while (queue_size_ > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void threadpool::shutdown()
{
    for (auto& thread : threads_)
    {
        {
            std::lock_guard<std::mutex> lock(thread->mutex);
            thread->run_loop = false;
            thread->cond.notify_one();
        }
        thread->thread->join();
    }
}


void threadpool::thread_main(threadpool::thread* self)
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(self->mutex);
        if (!self->run_loop)
            return;
        else if (self->queue.empty())
        {
            self->cond.wait(lock);
            continue;
        }

        assert(!self->queue.empty());

        action* next = self->queue.front();
        self->queue.pop();
        lock.unlock();

        next->perform();

        on_complete(next);

        queue_size_--;
    }
}

} // newsflash
