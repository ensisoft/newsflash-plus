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

#include <newsflash/config.h>
#include <boost/test/minimal.hpp>
#include <atomic>
#include <thread>
#include "../threadpool.h"
#include "../action.h"

// the threading test cases where will print out a lot of errors when ran with helgrind such as:
// ==2971== Possible data race during read of size 4 at 0x676AB4 by thread #4
// ==2971== Locks held: none
// ==2971==    at 0x45DFB7: boost::detail::get_once_per_thread_epoch() (once.cpp:61)
// ==2971==    by 0x4586A1: void boost::call_once<void (*)()>(boost::once_flag&, void (*)()) (once.hpp:69)
// ...
// <snip>
// these come from boost.tls in logging.cpp. the TLS object is disabled the diagnostics goes away.

void unit_test_pool()
{
    std::atomic_int counter {0};

    struct action : public newsflash::action
    {
    public:
        action(std::atomic_int& c) : counter_(c)
        {}

        virtual void xperform()
        {
            counter_++;
        }
    private:
        std::atomic_int& counter_;
    };

    newsflash::threadpool threads(4);
    threads.on_complete = [](newsflash::action* a) { delete a; };

    for (int i=0; i<10000; ++i)
    {
        auto* a = new action(counter);
        a->set_affinity(newsflash::action::affinity::any_thread);
        a->set_owner(i);
        threads.submit(a);
    }
    threads.wait_all_actions();
    threads.shutdown();

    BOOST_REQUIRE(counter == 10000);
}

void unit_test_private_thread()
{
    std::atomic_int generic_counter {0};
    std::atomic_int private_counter {0};

    struct generic_counter_action : public newsflash::action
    {
    public:
        generic_counter_action(std::atomic_int& c) : counter_(c)
        {}

        virtual void xperform()
        {
            counter_++;
        }
    private:
        std::atomic_int& counter_;
    };

    struct private_counter_action : public newsflash::action
    {
    public:
        private_counter_action(std::atomic_int& c) : counter_(c)
        {}

        virtual void xperform()
        {
            static auto id = std::this_thread::get_id();
            BOOST_REQUIRE(id == std::this_thread::get_id());
            counter_++;
        }
    private:
        std::atomic_int& counter_;
    };

    newsflash::threadpool threads(4);

    threads.on_complete = [](newsflash::action* a) { delete a; };

    auto* handle = threads.allocate();
    BOOST_REQUIRE(handle);

    for (int i=0; i<10000; ++i)
    {
        auto* a = new generic_counter_action(generic_counter);
        a->set_affinity(newsflash::action::affinity::any_thread);
        a->set_owner(i);
        threads.submit(a);

        auto* b = new private_counter_action(private_counter);
        threads.submit(b, handle);
    }
    threads.detach(handle);
    threads.wait_all_actions();
    threads.shutdown();

    BOOST_REQUIRE(generic_counter == 10000);
    BOOST_REQUIRE(private_counter == 10000);
}

int test_main(int, char*[])
{
    unit_test_pool();
    unit_test_private_thread();

    return 0;
}