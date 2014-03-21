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
#include <thread>
#include <functional>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include "../speedometer.h"

void produce_sample(int num_samples, int sample_bytes, corelib::speedometer& meter)
{
    std::srand(std::time(NULL));
    meter.start();

    for (int i=0; i<num_samples; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        const int bytes = 0.8 * sample_bytes + (rand() % (int)(sample_bytes * 0.2));

        assert(bytes >= (int)0.8 * sample_bytes && 
            bytes <= sample_bytes);

        meter.submit(bytes);
    }
}


void test_multiple_thread()
{
    corelib::speedometer meter;

    std::thread thread(std::bind(produce_sample, 100, 128, std::ref(meter)));

    // read the meter. 
    for (int i=0; i<200; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << meter.bps() << std::endl;
    }

    thread.join();
}


int test_main(int, char*[])
{
    test_multiple_thread();

    return 0;
}