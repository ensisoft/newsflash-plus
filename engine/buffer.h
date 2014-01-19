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

#pragma once

#include <boost/noncopyable.hpp>
#include <cassert>
#include <cstring>
#include "allocator.h"

namespace newsflash
{
    class buffer : boost::noncopyable
    {
    public:
        buffer() 
           : data_(nullptr), offset_(0), capacity_(0), size_(0)
        {}
       ~buffer()
        {
            allocator& alloc = allocator::get();
            alloc.free(data_, capacity_);
        }

        const void* read_ptr() const
        {
            return data_ + offset_;
        }

        void* write_ptr()
        {
            return data_ + size_;
        }

        const void* ptr() const 
        {
            return data_;
        }

        size_t available() const
        {
            return capacity_ - size_;
        }

        void grow(size_t capacity)
        {
            assert(capacity > capacity_);
            allocator& alloc = allocator::get();

            char* data = alloc.allocate(capacity);

            if (data_)
            {
                std::memcpy(data, data_, size_);
                alloc.free(data_, capacity_);
            }
            data_ = data;
            capacity_ = capacity;
        }

        void resize(size_t size)
        {
            assert(size <= capacity_);
            size_ = size;
        }
    private:
        char* data_;
        size_t offset_;
        size_t capacity_;
        size_t size_;
    };

} // newsflash
