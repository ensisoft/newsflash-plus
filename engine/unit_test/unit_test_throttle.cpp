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
#  include <boost/random/mersenne_twister.hpp>
#include "newsflash/warnpop.h"

#include <thread>
#include <iostream>
#include <atomic>
#include <cstdlib>
#include <ctime>

#include "engine/throttle.h"
#include "engine/platform.h"

int test_main(int, char*[])
{
    using clock_type = std::chrono::steady_clock;

    // single thread
    {
        std::srand(std::time(nullptr));

        newsflash::throttle throttle;
        throttle.enable(true);
        throttle.set_quota(1024 * 5); // 5kb/s

        const auto start = clock_type::now();

        std::size_t bytes_total = 0;

        for (;;)
        {
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now() - start);
            if (ms.count() >= 10000)
                break;

            const auto quota = throttle.give_quota();
            const auto bytes = std::rand() / (double)RAND_MAX  * quota;
            bytes_total += bytes;

            throttle.accumulate(bytes, quota);
        }
        // we've ran for 10s so max should 10 * 5kb
        BOOST_REQUIRE(bytes_total <= 10 * 1024 * 5);

        std::cout << "bytes received: " << bytes_total << " (" << bytes_total / 1024 << " kb)\n";

    }

    // multi thread
    {
        newsflash::throttle throttle;
        throttle.enable(true);
        throttle.set_quota(1024 * 10); // 10kb/s

        std::atomic<std::size_t> bytes_total(0);

        const auto start = clock_type::now();

        auto func = [&] {

            boost::random::mt19937 rand;
            rand.seed((unsigned)newsflash::get_thread_identity());

            for (;;)
            {
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now() - start);
                if (ms.count() >= 10000)
                    break;

                std::size_t quota = throttle.give_quota();
                while (!quota)
                {
                    const auto ms = rand() % 10;
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                    quota = throttle.give_quota();
                }

                const auto bytes = rand() / (double)rand.max() * quota;
                bytes_total += bytes;
                throttle.accumulate(bytes, quota);
            }
        };

        std::thread t1(func);
        std::thread t2(func);
        std::thread t3(func);

        t1.join();
        t2.join();
        t3.join();

        // we've ran for 10s  at 10kb/s so max should be 100kb
        BOOST_REQUIRE(bytes_total <= 10 * 1024 * 10);

        std::cout << "bytes received: " << bytes_total << " (" << bytes_total / 1024 << " kb)\n";
    }

    return 0;
}