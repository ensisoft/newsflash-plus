// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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
#include <boost/filesystem.hpp>
#include <thread>
#include "../logging.h"

namespace boost_fs = boost::filesystem;

void erase(const std::string& f)
{
    boost_fs::path p(f);
    boost_fs::remove(p);
}

void thread_entry()
{
    LOG_OPEN("foo", "thread-foo.log");
    LOG_OPEN("bar", "thread-bar.log");

    LOG_D("This is a BAR debug");
    LOG_E("This is a BAR error");
    LOG_I("This is a BAR information");

    LOG_SELECT("foo");
    LOG_D("This is a FOO debug");
    LOG_E("This is a FOO error");
    LOG_I("This is a FOO information");
    LOG_CLOSE();
}

int test_main(int, char*[])
{
    erase("thread-foo.log");
    erase("thread-bar.log");
    erase("foo.log");
    erase("bar.log");

    std::thread thread(thread_entry);

    LOG_OPEN("foo", "foo.log");
    LOG_OPEN("bar", "bar.log");

    LOG_D("This is BAR debug");
    LOG_E("This is BAR error");
    LOG_E("This is BAR information");


    LOG_SELECT("foo");
    LOG_D("This is FOO debug");
    LOG_E("This is FOO error");
    LOG_E("this is FOO information");

    thread.join();
    return 0;
}