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
#include "assert.h"

namespace newsflash
{
    // id tagged data buffer
    class buffer : boost::noncopyable
    {
    public:
        typedef uint8_t byte_t;

        // construct an empty buffer without any data capacity
        buffer() : data_(nullptr), offset_(0), capacity_(0), size_(0)
        {}

        // construct an empty buffer with some initial data capacity 
        explicit
        buffer(std::size_t capacity) : buffer()
        {
            allocate(capacity);
        }

        // construct a buffer from the contents of the NUL terminated string.
        explicit 
        buffer(const char* str) : buffer()
        {
            const std::size_t s = std::strlen(str);

            allocate(s);
            std::memcpy(data_, str, s);
            size_ = s;
        }

        buffer(buffer&& other)
        {
            data_     = steal(other.data_);
            offset_   = steal(other.offset_);
            capacity_ = steal(other.capacity_);
            size_     = steal(other.size_);            
        }

       ~buffer()
        {
            if (data_)
            {
                const auto bird = CANARY;
                ASSERT(!std::memcmp(data_ + capacity_, &bird, sizeof(bird)) &&
                    "Buffer overrun detected.");
            }
            allocator& alloc = allocator::get();
            alloc.free(data_, capacity_);
        }
        const byte_t* ptr() const 
        {
            return data_;
        }

        byte_t* ptr()
        {
            return data_;
        }
        std::size_t capacity() const
        {
            return capacity_;
        }
        std::size_t size() const
        {
            return size_;
        }
        std::size_t offset() const
        {
            return offset_;
        }

        void allocate(size_t capacity)
        {
            assert(capacity > capacity_);
            allocator& alloc = allocator::get();

            const auto bird = CANARY;

            byte_t* data = (byte_t*)alloc.allocate(capacity + sizeof(bird));

            if (data_)
            {
                ASSERT(!std::memcmp(data_ + capacity_, &bird, sizeof(bird)) && 
                    "Buffer overrun detected");

                std::memcpy(data, data_, size_);
                alloc.free(data_, capacity_);
            }
            data_ = data;
            capacity_ = capacity;

            std::memcpy(data_ + capacity_, &bird, sizeof(bird));
        }

        void configure(std::size_t size, std::size_t offset)
        {
            assert(offset + size <= capacity_);
            assert(offset <= size);
            offset_ = offset;
            size_   = size;
        }

        void resize(std::size_t size)
        {
            configure(size, offset_);
        }


        byte_t& operator[](std::size_t i)
        {
            assert(i < capacity_);
            return data_[i];
        }
        const byte_t& operator[](std::size_t i) const
        {
            assert(i < capacity_);
            return data_[i];
        }

        buffer& operator=(buffer&& other)
        {
            if (this == &other)
                return *this;

            buffer tmp(std::move(*this));
            data_     = steal(other.data_);
            offset_   = steal(other.offset_);
            capacity_ = steal(other.capacity_);
            size_     = steal(other.size_);
            return *this;
        }

        bool empty() const
        {
            return (size_ - offset_ ==  0);
        }

    private:
        template<typename T>
        T steal(T& loot) const
        {
            T val = loot;
            loot  = T();
            return val;
        }

        enum : uint32_t { CANARY= 0xcafebabe };

        byte_t* data_;
        std::size_t offset_; // offset into the buffer where "real" data begins
        std::size_t capacity_; // current buffer capacity, i.e. amount of bytes pointed by data_
        std::size_t size_; // current total data size
    };

    inline
    std::size_t buffer_capacity(const buffer& buff)
    {
        return buff.capacity();
    }

    inline
    void* buffer_data(buffer& buff)
    {
        return buff.ptr();
    }

    inline
    void grow_buffer(buffer& buff, size_t capacity)
    {
        buff.allocate(capacity);
    }

    inline
    const buffer::byte_t* buffer_payload(const buffer& buff)
    {
        return buff.ptr() + buff.offset();
    }

    inline
    std::size_t buffer_payload_size(const buffer& buff)
    {
        return buff.size() - buff.offset();
    }


    inline
    std::size_t buffer_size(const buffer& buff)
    {
        return buff.size();
    }

    inline
    bool operator==(const buffer& lhs, const buffer& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        return !std::memcmp(lhs.ptr(), rhs.ptr(), lhs.size());
    }

} // newsflash
