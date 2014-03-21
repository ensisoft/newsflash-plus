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

#include <boost/test/minimal.hpp>
#include <atomic>
#include "../threadpool.h"

std::atomic_int counter;

struct counter_increment : public newsflash::threadpool::work
{
    void execute()
    {
        counter++;
    }
};


struct affinity_tester : public newsflash::threadpool::work
{
    affinity_tester(std::thread::id id) : tid(id)
    {}

    void execute()
    {
        BOOST_REQUIRE(tid == std::this_thread::get_id());
    }
    const std::thread::id tid;
};

struct affinity_grabber : public newsflash::threadpool::work
{
    affinity_grabber() : ready(false)
    {}

    void execute()
    {
        std::lock_guard<std::mutex> lock(mutex);
        tid = std::this_thread::get_id();
        ready = true;
        cond.notify_one();
    }
    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [&]() { return ready; });
    }

    std::thread::id tid;
    std::mutex mutex;
    std::condition_variable cond;
    bool ready;
};

void test_work_queue()
{
    newsflash::threadpool pool(1);

    for (int i=0; i<5000; ++i)
    {
        auto tid = pool.allocate();
        pool.submit(std::unique_ptr<counter_increment>(new counter_increment), tid);
    }

    pool.drain();

    BOOST_REQUIRE(counter == 5000);
}

void test_affinity()
{
    newsflash::threadpool pool(5);

    affinity_grabber grabber;

    const auto thread = pool.allocate();

    pool.submit(&grabber, thread);
    grabber.wait();

    for (int i=0; i<5000; ++i)
    {
        std::unique_ptr<affinity_tester> work(new affinity_tester(grabber.tid));
        pool.submit(std::move(work), thread);
    }

}

int test_main(int, char*[])
{
    test_work_queue();
    test_affinity();

    return 0;
}