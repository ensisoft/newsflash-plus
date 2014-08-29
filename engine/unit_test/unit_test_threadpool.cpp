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

#include <newsflash/config.h>

#include <boost/test/minimal.hpp>
#include <atomic>
#include "../threadpool.h"
#include "../action.h"

int test_main(int, char*[])
{
    std::atomic_int counter;

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

    newsflash::threadpool threads(5);
    threads.on_complete = [](newsflash::action* a) { delete a; };

    for (int i=0; i<10000; ++i)
    {
        threads.submit(new action(counter));
    }

    threads.wait();

    BOOST_REQUIRE(counter == 10000);


    return 0;
}