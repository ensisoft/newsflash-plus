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
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include <chrono>
#include "../waithandle.h"
#include "../event.h"
#include "../msgqueue.h"

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

void unit_test_event()
{
    {
        newsflash::event event;

        auto handle = event.wait();
        BOOST_REQUIRE(!wait_for(handle, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(!handle.read());

        event.set();
        handle = event.wait();
        BOOST_REQUIRE(wait_for(handle, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(handle.read());

        event.reset();
        handle = event.wait();
        BOOST_REQUIRE(!wait_for(handle, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(!handle.read());

    }    

    {
        newsflash::event event1, event2;

        auto handle1 = event1.wait();
        auto handle2 = event2.wait();

        BOOST_REQUIRE(!wait_for(handle1, handle2, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(!handle1.read());
        BOOST_REQUIRE(!handle2.read());

        event2.set();

        handle1 = event1.wait();
        handle2 = event2.wait();
        BOOST_REQUIRE(wait_for(handle1, handle2, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(!handle1.read());
        BOOST_REQUIRE(handle2.read());

        event2.reset();
        event1.set();

        handle1 = event1.wait();
        handle2 = event2.wait();
        BOOST_REQUIRE(wait_for(handle1, handle2, std::chrono::milliseconds(0)));
        BOOST_REQUIRE(handle1.read());
        BOOST_REQUIRE(!handle2.read());


    }

    const auto& waiter = [](newsflash::event& event, barrier& bar, std::atomic_flag& flag)
    {
        // rendezvous
        bar.wait();
        
        // begin wait forever
        auto handle = event.wait();
        wait_for(handle);
        
        BOOST_REQUIRE(handle.read());
        
        // expected to be true after wait completes
        BOOST_REQUIRE(flag.test_and_set());        
        
    };
    
    // single thread waiter
    {
        for (int i=0; i<1000; ++i)
        {
            newsflash::event event;

            barrier bar(2);

            std::atomic_flag flag;

            std::thread th(std::bind(waiter, std::ref(event), std::ref(bar), std::ref(flag)));

            // rendezvous with the other thread
            bar.wait();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // set to true before signaling
            flag.test_and_set();

            // release the waiters
            event.set();

            th.join();
        }
    }

    // multiple threads waiting.
    {
        for (int i=0; i<1000; ++i)
        {
            newsflash::event event;

            barrier bar(4);

            std::atomic_flag flag;

            std::thread th1(std::bind(waiter, std::ref(event), std::ref(bar), std::ref(flag)));
            std::thread th2(std::bind(waiter, std::ref(event), std::ref(bar), std::ref(flag)));
            std::thread th3(std::bind(waiter, std::ref(event), std::ref(bar), std::ref(flag)));

            // rendezvous with all the threads
            bar.wait();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            flag.test_and_set();

            event.set();

            th1.join();
            th2.join();
            th3.join();
        }
    }

}

struct foo {
    int i;
    std::string s;
};

// test posting messages to the queue.
// posting is an async operation and returns immediately.
void unit_test_queue_post()
{
    {
        std::unique_ptr<newsflash::msgqueue::message> msg;

        newsflash::msgqueue queue;
        BOOST_REQUIRE(queue.size() == 0);
        auto front = queue.try_get_front();
        BOOST_REQUIRE(!front);

     
        queue.post_back(123, foo{444, "jallukola"});
        BOOST_REQUIRE(queue.size() == 1);

        auto m = queue.get_front();
        BOOST_REQUIRE(m->id() == 123);
        BOOST_REQUIRE(m->type_test<foo>());
        BOOST_REQUIRE(m->as<foo>().i == 444);
        BOOST_REQUIRE(m->as<foo>().s == "jallukola");
        BOOST_REQUIRE(queue.size() == 0);

        // lvalue
        foo lvalue {1234, "jallukola"};
        queue.post_back(1, lvalue);

        // call to q.post should not modify f
        BOOST_REQUIRE(lvalue.i == 1234);
        BOOST_REQUIRE(lvalue.s == "jallukola");
        BOOST_REQUIRE(queue.size() == 1);

        m = queue.get_front();
        BOOST_REQUIRE(m->type_test<foo>());
        BOOST_REQUIRE(m->as<foo>().i == 1234);
        BOOST_REQUIRE(m->as<foo>().s == "jallukola");
        BOOST_REQUIRE(queue.size() == 0);

    }

    {
        newsflash::msgqueue queue;
        queue.post_back(1, std::string{"foo"});
        queue.post_back(2, std::string{"bar"});
        queue.post_front(3, std::string{"baz"});

        BOOST_REQUIRE(queue.size() == 3);
        auto m = queue.get_front();
        BOOST_REQUIRE(m->id() == 3);
        BOOST_REQUIRE(m->as<std::string>() == "baz");
        BOOST_REQUIRE(queue.size() == 2);
        queue.post_front(m);
        BOOST_REQUIRE(queue.size() == 3);
        m = queue.get_front();
        BOOST_REQUIRE(m->id() == 3);
        m = queue.get_front();
        BOOST_REQUIRE(m->id() == 1);
        m = queue.get_front();
        BOOST_REQUIRE(m->id() == 2);
        BOOST_REQUIRE(queue.size() == 0);

    }

    // try posting primitve type message
    {
        newsflash::msgqueue queue;

        for (size_t i=0; i<10000; ++i)
        {
            const size_t id = i + 1;
            const size_t data = i + 2;
            queue.post_back(id, data);
        }
        BOOST_REQUIRE(queue.size() == 10000);

        for (size_t i=0; i<10000; ++i)
        {
            const size_t id = i + 1;
            const size_t data = i + 2;

            auto msg = queue.get_front();
            BOOST_REQUIRE(msg->id() == id);
            BOOST_REQUIRE(msg->as<size_t>() == data);
        }

    }

    // try posting messages of user defined type
    {
        newsflash::msgqueue queue;

        const foo msg {1234, "foobar"};

        for (size_t i=0; i<10000; ++i)
        {
            queue.post_back(1, msg);
        }

        BOOST_REQUIRE(queue.size() == 10000);

        for (size_t i=0; i<10000; ++i)
        {
            auto m = queue.get_front();
            BOOST_REQUIRE(m->id() == 1);
            BOOST_REQUIRE(m->as<foo>().s == "foobar");
            BOOST_REQUIRE(m->as<foo>().i == 1234);
        }

    }

    // routine to consume messages out of the queue.
    const auto& consumer = [](newsflash::msgqueue& queue, int count) 
    {
        for (int i=0; i<count; ++i)
        {
            const auto msg = queue.get_front();
            BOOST_REQUIRE(msg->id() == 1234);
            BOOST_REQUIRE(msg->as<foo>().s == "foobar");
            BOOST_REQUIRE(msg->as<foo>().i == 1234567);
        }
    };

    // routine to produce messages into the queue
    const auto& producer = [](newsflash::msgqueue& queue, int count)
    {
        for (int i=0; i<count; ++i)
        {
            queue.post_back(1234, foo { 1234567, "foobar"});
        }
    };

    // single producer and consumer
    {
        newsflash::msgqueue queue;

        std::thread consumer1(std::bind(consumer, std::ref(queue), 10000));
        std::thread producer1(std::bind(producer, std::ref(queue), 10000));

        consumer1.join();
        producer1.join();
    }

    // single producer and multiple consumers
    {
        newsflash::msgqueue queue;

        std::thread consumer1(std::bind(consumer, std::ref(queue), 5000));
        std::thread consumer2(std::bind(consumer, std::ref(queue), 5000));
        std::thread producer1(std::bind(producer, std::ref(queue), 10000));

        producer1.join();
        consumer1.join();
        consumer2.join();
    }

    // multiple producers and multiple consumers
    {
        newsflash::msgqueue queue;

        std::thread consumer1(std::bind(consumer, std::ref(queue), 5000));
        std::thread consumer2(std::bind(consumer, std::ref(queue), 5000));
        std::thread producer1(std::bind(producer, std::ref(queue), 5000));
        std::thread producer2(std::bind(producer, std::ref(queue), 5000));

        producer1.join();
        producer2.join();
        consumer1.join();
        consumer2.join();
    }
}

// try sending messages the queue.
// sending is a synchronous operation and will block
// untill the message has been read and discarded by a receiving thread.
// this can be used to request data to be returned back in the message
void unit_test_queue_send()
{
    typedef std::string message;

    const auto& receiver = [](newsflash::msgqueue& queue, int count)
    {
        for (int i=0; i<count; ++i)
        {
            auto msg = queue.get_front();
            BOOST_REQUIRE(msg->id() == 1);
            msg->as<message>() = "foobar";
        }
    };

    const auto& sender = [](newsflash::msgqueue& queue, int count)
    {
        for (int i=0; i<count; ++i)
        {
            message m;
            queue.send_back(1, m);
            BOOST_REQUIRE(m == "foobar");
        }
    };

    // single sender and receiver
    {
        newsflash::msgqueue queue;

        std::thread receiver1(std::bind(receiver, std::ref(queue), 10000));
        std::thread sender1(std::bind(sender, std::ref(queue), 10000));

        receiver1.join();
        sender1.join();
    }

    // multiple senders and receivers
    {
        newsflash::msgqueue queue;

        std::thread receiver1(std::bind(receiver, std::ref(queue), 5000));
        std::thread receiver2(std::bind(receiver, std::ref(queue), 5000));        
        std::thread sender1(std::bind(sender, std::ref(queue), 5000));
        std::thread sender2(std::bind(sender, std::ref(queue), 5000));        

        receiver1.join();
        receiver2.join();
        sender1.join();
        sender2.join();
    }

}


int test_main(int, char*[])
{
    unit_test_event();
    unit_test_queue_post();
    unit_test_queue_send();
    return 0;
}

