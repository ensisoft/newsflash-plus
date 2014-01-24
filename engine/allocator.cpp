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

#include <cstdlib>
#include <cassert>
#include "allocator.h"

namespace newsflash
{

allocator::allocator() : volume_(0), count_(0), allocated_(0)
{}

allocator::~allocator()
{
    // all slabs should have been returned
    assert(allocated_ == 0);

    for (const auto& s : slabs_)
    {
        std::free(s.second);
    }
}

void* allocator::allocate(size_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);

    void* ret;

    // see if already have a slab that satisfies
    // the size requirement. if we have, grab it and
    // erase it from the map of available slabs.
    // otherwise allocate a new slab.

    // todo: total mem consumption limit, how many slabs, how many bytes?

    const auto& it = slabs_.lower_bound(size);
    if (it == slabs_.end())
    {
        ret = std::malloc(size);
        count_++;
        volume_ += size;
    }
    else
    {
        ret = it->second;
        slabs_.erase(it);
    }
    allocated_++;
    return ret;
}

void allocator::free(void* ptr, size_t size)
{
    if (!ptr)
        return;

    assert(size);

    std::lock_guard<std::mutex> lock(mutex_);

    slabs_.insert(std::make_pair(size, ptr));
    allocated_--;
}

allocator& allocator::get()
{
    static allocator alloc; // thread safe per c++11 rules
    return alloc;
}


} // newsflash
