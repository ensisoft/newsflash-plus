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

#pragma once

#include <newsflash/config.h>
#include <cstddef>
#include <cstdint>
#include <cstring> // for memcpy
#include <cassert>

#define ASSERT assert

namespace nntp
{
    class response
    {
    public:
        enum class status {
            none, success, dmca, unavailable
        };

        enum class type {
            none, list, body, xover
        };

        class segment
        {
        public:
            segment(const char* ptr, std::size_t len) 
                :  ptr_(ptr), len_(len)
                {}

        private:
            const char* ptr_;
            std::size_t len_;

        };

        response() : status_(status::none), type_(type::none), data_(nullptr), 
            capacity_(0), size_(0), content_offset_(0)
        {}

        explicit
        response(std::size_t capacity) : response() 
        {
            allocate(capacity);
        }

       ~response()
        {
            if (data_)
            {
                ASSERT(!std::memcmp(data_ + capacity_, &CANARY, sizeof(CANARY)) &&
                    "buffer overrun detected");
            }
            operator delete(data_);
        }

        void allocate(std::size_t capacity)
        {
            if (capacity <= capacity_)
                return;
            auto data = (char*)operator new(capacity + sizeof(CANARY));
            if (data_)
            {
                ASSERT(!std::memcmp(data_ + capacity_, &CANARY, sizeof(CANARY)) &&
                    "buffer overrun detected");
                std::memcpy(data, data_, size_);
                operator delete(data_);
            }
            data_     = data;
            capacity_ = capacity;
            std::memcpy(data_ + capacity_, &CANARY, sizeof(CANARY));
        }

        void clear()
        {
            size_ = 0;
            content_offset_ = 0;
            type_ = type::none;
            status_ = status::none;
        }

        void resize(std::size_t size)
        {
            ASSERT(size <= capacity_);
            size_ = size;
        }

        char* data() 
        { return data_; }

        const char* data() const 
        { return data_; }

        std::size_t capacity() const 
        { return capacity_; }

        std::size_t size() const 
        { return size_; }

        segment body() const
        {
            return segment { data_ + content_offset_, 
                size_ - content_offset_};
        }
        segment header() const 
        {
            return segment { data_, content_offset_ };
        }



    private:
        const static std::uint32_t CANARY { 0xcafebabe };

    private:
        status status_;
        type   type_;
        char* data_;
        std::size_t capacity_; // current response capacity, i.e. amount of bytes pointed by data_
        std::size_t size_; // current response data size
        std::size_t content_offset_; // offset into data where content begins
    };
} // nntp
