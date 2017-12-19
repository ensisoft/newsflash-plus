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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <boost/test/minimal.hpp>
#include "newsflash/warnpop.h"

#include <condition_variable>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include <chrono>

#include "engine/waithandle.h"
#include "engine/event.h"

class barrier
{
public:
    barrier(std::size_t count) : count_(count)
    {}

    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (--count_ == 0)
        {
            cond_.notify_all();
            return;
        }
        while (count_)
            cond_.wait(lock);
    }
private:
    std::mutex mutex_;
    std::condition_variable cond_;
    std::size_t count_;
};

void unit_test_event()
{
    {
        newsflash::Event event;

        auto handle = event.GetWaitHandle();
        BOOST_REQUIRE(!WaitForSingleHandle(handle, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(!handle.CanRead());

        event.SetSignal();
        handle = event.GetWaitHandle();
        BOOST_REQUIRE(WaitForSingleHandle(handle, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(handle.CanRead());

        event.ResetSignal();
        handle = event.GetWaitHandle();
        BOOST_REQUIRE(!WaitForSingleHandle(handle, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(!handle.CanRead());

    }

    {
        newsflash::Event event1, event2;

        auto handle1 = event1.GetWaitHandle();
        auto handle2 = event2.GetWaitHandle();

        BOOST_REQUIRE(!WaitForMultipleHandles(handle1, handle2, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(!handle1.CanRead());
        BOOST_REQUIRE(!handle2.CanRead());

        event2.SetSignal();

        handle1 = event1.GetWaitHandle();
        handle2 = event2.GetWaitHandle();
        BOOST_REQUIRE(WaitForMultipleHandles(handle1, handle2, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(!handle1.CanRead());
        BOOST_REQUIRE(handle2.CanRead());

        event2.ResetSignal();
        event1.SetSignal();

        handle1 = event1.GetWaitHandle();
        handle2 = event2.GetWaitHandle();
        BOOST_REQUIRE(WaitForMultipleHandles(handle1, handle2, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(handle1.CanRead());
        BOOST_REQUIRE(!handle2.CanRead());


    }

    const auto& waiter = [](newsflash::Event& event, barrier& bar, std::atomic_flag& flag)
    {
        // rendezvous
        bar.wait();

        // begin wait forever
        auto handle = event.GetWaitHandle();
        WaitForSingleHandle(handle);

        BOOST_REQUIRE(handle.CanRead());

        // expected to be true after wait completes
        BOOST_REQUIRE(flag.test_and_set());

    };

    // single thread waiter
    {
        for (int i=0; i<1000; ++i)
        {
            newsflash::Event event;

            barrier bar(2);

            std::atomic_flag flag;

            std::thread th(std::bind(waiter, std::ref(event), std::ref(bar), std::ref(flag)));

            // rendezvous with the other thread
            bar.wait();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // set to true before signaling
            flag.test_and_set();

            // release the waiters
            event.SetSignal();

            th.join();
        }
    }

    // multiple threads waiting.
    {
        for (int i=0; i<1000; ++i)
        {
            newsflash::Event event;

            barrier bar(4);

            std::atomic_flag flag;

            std::thread th1(std::bind(waiter, std::ref(event), std::ref(bar), std::ref(flag)));
            std::thread th2(std::bind(waiter, std::ref(event), std::ref(bar), std::ref(flag)));
            std::thread th3(std::bind(waiter, std::ref(event), std::ref(bar), std::ref(flag)));

            // rendezvous with all the threads
            bar.wait();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            flag.test_and_set();

            event.SetSignal();

            th1.join();
            th2.join();
            th3.join();
        }
    }

}

int test_main(int, char*[])
{
    unit_test_event();
    return 0;
}

