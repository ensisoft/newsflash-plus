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
#include "iothread.h"
#include "ioaction.h"
#include "taskio.h"

namespace newsflash
{

iothread::iothread(ioqueue& in, ioqueue& out) 
    : ioin_(in), ioout_(out), 
      thread_(std::bind(&iothread::main, this))
{}

iothread::~iothread()
{
    shutdown_.set();
    thread_.join();
}

void iothread::main()
{
    while (true)
    {
        auto io = ioin_.wait();
        auto shutdown  = shutdown_.wait();
        newsflash::wait_for(io, shutdown);

        if (shutdown)
            break;

        auto action = ioin_.get_front();

        action->perform();

        ioout_.push_back(std::move(action));
    }
}

} // newsflash