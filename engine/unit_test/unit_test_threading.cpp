// Copyright (c) 2013 Sami Väisänen, Ensisoft 
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
#include <newsflash/engine/msgqueue.h>
#include <newsflash/engine/event.h>
#include <newsflash/engine/platform.h>
#include <newsflash/engine/assert.h>
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include <chrono>


struct foo {
    int i;
    std::string s;
};

struct bar {
    std::vector<int> v;
};

namespace nt = newsflash;

class barrier 
{
public:
    barrier(size_t count) : count_(count)
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
    size_t count_;    
};



void thread_func_test_event(nt::event* e, barrier* b, std::atomic_flag* flag)
{
    // rendezvous
    b->wait();

    // wait on the event
    e->wait();

    // expected to be true after the wait completes
    BOOST_REQUIRE(flag->test_and_set());
}

void unit_test_event()
{
    {
        nt::event ev;

        BOOST_REQUIRE(!ev.is_set());
        BOOST_REQUIRE(!nt::is_signaled(ev.handle()));
        ev.set();
        BOOST_REQUIRE(ev.is_set());
        BOOST_REQUIRE(nt::is_signaled(ev.handle()));
        ev.wait();
        ev.reset();
        BOOST_REQUIRE(!ev.is_set());
        BOOST_REQUIRE(!nt::is_signaled(ev.handle()));
    }    
    
    // single thread waiter
    {
        for (int i=0; i<1000; ++i)
        {
            nt::event ev;

            barrier bar(2);

            std::atomic_flag flag;

            std::thread th(std::bind(thread_func_test_event, &ev, &bar, &flag));

            // rendezvous with the other thread
            bar.wait();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // set to true before signaling
            flag.test_and_set();

            // release the waiters
            ev.set();

            th.join();
        }
    }

    // multiple threads waiting.
    {
        for (int i=0; i<1000; ++i)
        {
            nt::event ev;

            barrier bar(4);

            std::atomic_flag flag;

            std::thread th1(std::bind(thread_func_test_event, &ev, &bar, &flag));
            std::thread th2(std::bind(thread_func_test_event, &ev, &bar, &flag));
            std::thread th3(std::bind(thread_func_test_event, &ev, &bar, &flag));

            // rendezvous with all the threads
            bar.wait();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            flag.test_and_set();

            ev.set();

            th1.join();
            th2.join();
            th3.join();
        }
    }

}

void thread_func_test_queue(nt::msgqueue* q, int msg_count)
{
    for (int i=0; i<msg_count; ++i)
    {
        const auto msg = q->get();
        BOOST_REQUIRE(msg->as<int>() == 123);
        BOOST_REQUIRE(msg->id() == 666);
    }
}

void thread_func_test_queue2(nt::msgqueue* q, int msg_count)
{
    for (int i=0; i<msg_count; ++i)
    {
        const auto msg = q->get();
        std::string* str = msg->as<std::string*>();
        *str = "jallukola";
    }
}

void unit_test_queue()
{
    {
        nt::msgqueue q;

        BOOST_REQUIRE(q.size() == 0);
        BOOST_REQUIRE(q.handle());
        BOOST_REQUIRE(!nt::is_signaled(q.handle()));

        q.post(123, foo{444, "jallukola"});

        BOOST_REQUIRE(q.size() == 1);
        BOOST_REQUIRE(nt::is_signaled(q.handle()));

        auto m = q.get();
        BOOST_REQUIRE(m->id() == 123);
        BOOST_REQUIRE(m->type_test<foo>());
        BOOST_REQUIRE(m->as<foo>().i == 444);
        BOOST_REQUIRE(m->as<foo>().s == "jallukola");

        BOOST_REQUIRE(q.size() == 0);
        BOOST_REQUIRE(!nt::is_signaled(q.handle()));

        // lvalue
        foo f {1234, "jallukola"};

        q.post(1, f);

        // call to q.post should not modify f
        BOOST_REQUIRE(f.i == 1234);
        BOOST_REQUIRE(f.s == "jallukola");

        BOOST_REQUIRE(q.size() == 1);
        m = q.get();
        BOOST_REQUIRE(m->type_test<foo>());
        BOOST_REQUIRE(m->as<foo>().i == 1234);
        BOOST_REQUIRE(m->as<foo>().s == "jallukola");

    }

    {
        nt::msgqueue q;

        for (size_t i=0; i<10000; ++i)
        {
            q.post(i + 1, i + 2);
        }

        BOOST_REQUIRE(q.size() == 10000);

        for (size_t i=0; i<10000; ++i)
        {
            auto m = q.get();
            BOOST_REQUIRE(m->id() == i + 1);
            BOOST_REQUIRE(m->as<size_t>() == i + 2);
        }
    }

    {
        nt::msgqueue q;

        foo f {123, "foobar"};
        q.post(321, f);

        BOOST_REQUIRE(f.s == "foobar");
        BOOST_REQUIRE(f.i == 123);
        BOOST_REQUIRE(q.size() == 1);

        auto msg = q.get();
        BOOST_REQUIRE(msg->id() == 321);
        BOOST_REQUIRE(msg->as<foo>().s == "foobar");
        BOOST_REQUIRE(msg->as<foo>().i == 123);
    }

    // single consumer and producer
    {
        nt::msgqueue q;

        std::thread th(std::bind(thread_func_test_queue, &q, 1000));

        for (int i=0; i<1000; ++i)
            q.post(666, 123);

        th.join();
    }

    // multiple threads
    {
        nt::msgqueue q;

        std::thread th1(std::bind(thread_func_test_queue, &q, 5000));
        std::thread th2(std::bind(thread_func_test_queue, &q, 5000));

        for (int i=0; i<10000; ++i)
            q.post(666, 123);

        th1.join();
        th2.join();
    }

    // test waiting on a message, single consumer
    {
        nt::msgqueue q;

        std::thread th1(std::bind(thread_func_test_queue2, &q, 50000));

        for (int i=0; i<50000; ++i)
        {
            std::string str;
            // this should block untill the message has been processed 
            // and disposed of by the consumer.
            q.send(123, &str);

            BOOST_REQUIRE(str == "jallukola");
        }

        th1.join();
    }

    // test witing on a message, multiple consumers
    {
        nt::msgqueue q;

        std::thread th1(std::bind(thread_func_test_queue2, &q, 10000));
        std::thread th2(std::bind(thread_func_test_queue2, &q, 10000));
        std::thread th3(std::bind(thread_func_test_queue2, &q, 10000));

        for (int i=0; i<3 * 10000; ++i)
        {
            std::string str;
            q.send(123, &str);

            BOOST_REQUIRE(str == "jallukola");
        }

        th1.join();
        th2.join();
        th3.join();
    }
}

void unit_test_waiting()
{
    // test single object expect without blocking
    {
        nt::event ev;

        BOOST_REQUIRE(!nt::is_signaled(ev.handle()));
        BOOST_REQUIRE(!nt::wait_single_object(ev.handle(), std::chrono::milliseconds(0)));

        ev.set();
        BOOST_REQUIRE(nt::is_signaled(ev.handle()));
        BOOST_REQUIRE(nt::wait_single_object(ev.handle(), std::chrono::milliseconds(0)));

        ev.reset();
        BOOST_REQUIRE(!nt::is_signaled(ev.handle()));
        BOOST_REQUIRE(!nt::wait_single_object(ev.handle(), std::chrono::milliseconds(0)));
    }

    // test single object with blocking
    {
        nt::event ev;
        std::atomic_flag f;
        barrier bar(2);

        std::thread th([&]()
        {
            bar.wait();

            nt::wait_single_object(ev.handle());

            BOOST_REQUIRE(f.test_and_set());
        });

        bar.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        f.test_and_set();

        ev.set();

        th.join();
    }

    // test single object with blocking
    {
        nt::event ev;
        std::atomic_flag f;
        barrier bar(2);

        std::thread th([&]()
        {
            bar.wait();

            BOOST_REQUIRE(nt::wait_single_object(ev.handle(), std::chrono::milliseconds(1000)));
            BOOST_REQUIRE(f.test_and_set());
        });

        bar.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        f.test_and_set();

        ev.set();

        th.join();
    }

    // test single object with blocking and timeout
    {
        nt::event ev;
        barrier bar(2);

        std::thread th([&]()
        {
            bar.wait();

            BOOST_REQUIRE(!nt::wait_single_object(ev.handle(), std::chrono::milliseconds(100)));
        });

        bar.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        th.join();
    }

    // test multiple objects without blocking
    {
        nt::event e1, e2;

        BOOST_REQUIRE(nt::wait_multiple_objects({e1.handle(), e2.handle()}, std::chrono::milliseconds(0)) == nt::OS_INVALID_HANDLE);

        e1.set();

        BOOST_REQUIRE(nt::wait_multiple_objects({e1.handle(), e2.handle()}, std::chrono::milliseconds(0)) == e1.handle());
        BOOST_REQUIRE(nt::wait_multiple_objects({e1.handle(), e2.handle()}) == e1.handle());

    }

    // test multiple objects with blocking
    {
        nt::event e1, e2;
        std::atomic_flag f;
        barrier bar(2);

        std::thread th([&]()
        {
            bar.wait();

            BOOST_REQUIRE(nt::wait_multiple_objects({e1.handle(), e2.handle()}) == e1.handle());
            BOOST_REQUIRE(f.test_and_set());
        });

        bar.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        f.test_and_set();

        e1.set();

        th.join();
    }

    // test multiple objects with timeout
    {
        nt::event e1, e2;
        barrier bar(2);

        std::thread th([&]()
        {
            bar.wait();
            BOOST_REQUIRE(nt::wait_multiple_objects({e1.handle(), e2.handle()}, std::chrono::milliseconds(100)) == nt::OS_INVALID_HANDLE);
        });
        bar.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        th.join();
    }
}


int test_main(int, char*[])
{
    unit_test_event();
    unit_test_queue();
    unit_test_waiting();
    return 0;
}
