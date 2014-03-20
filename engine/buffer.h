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

#include <newsflash/config.h>

#include <boost/noncopyable.hpp>
#include <cassert>
#include <cstring>
#include <cstdint>
#include "allocator.h"
#include "assert.h"

namespace newsflash
{
    // nntp data buffer
    class buffer : boost::noncopyable
    {
    public:
        class header
        {
        public:
            header(const buffer& buff) : buffer_(buff)
            {}
            const char* data() const
            {
                return &buffer_[0];
            }
            std::size_t size() const
            {
                return buffer_.offset();
            }
        private:
            const buffer& buffer_;
        };

        class payload
        {
        public:
            payload(const buffer& buff) : buffer_(buff), crop_(0)
            {}

            const char* data() const
            {
                return &buffer_[buffer_.offset() + crop_];
            }
            std::size_t size() const
            {
                return buffer_.size() - (buffer_.offset() + crop_);
            }
            void crop(std::size_t bytes)
            {
                crop_ += bytes;
            }
            bool empty() const
            {
                return size() == 0;
            }
        private:
            const buffer& buffer_;
            std::size_t crop_;

        };

        // construct an empty buffer without any data capacity
        buffer() : data_(nullptr), capacity_(0), size_(0), offset_(0)
        {}

        // construct an empty buffer with some initial data capacity 
        explicit
        buffer(std::size_t capacity) : buffer()
        {
            reserve(capacity);
        }

        // construct a buffer from the contents of the NUL terminated string.
        explicit 
        buffer(const char* str) : buffer()
        {
            const std::size_t s = std::strlen(str);

            reserve(s);
            std::memcpy(data_, str, s);
            size_ = s;
        }

        buffer(const void* data, std::size_t len) : buffer()
        {
            reserve(len);
            std::memcpy(data_, data, len);
            size_ = len;
        }

        buffer(buffer&& other)
        {
            data_     = steal(other.data_);
            capacity_ = steal(other.capacity_);
            size_     = steal(other.size_);            
            offset_   = steal(other.offset_);
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

        char* data()
        {
            return data_;
        }
        const char* data() const
        {
            return data_;
        }

        void reserve(std::size_t capacity)
        {
            assert(capacity > capacity_);
            allocator& alloc = allocator::get();

            const auto bird = CANARY;

            auto data = (char*)alloc.allocate(capacity + sizeof(bird));
#ifdef NEWSFLASH_DEBUG
            memset(data, 0, capacity);
#endif

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

        void resize(std::size_t size)
        {
            if (size > capacity_)
                reserve(size);
            size_ = size;
        }

        void offset(std::size_t off)
        {
            offset_ = off;
        }

        char& operator[](std::size_t i)
        {
            assert(i < size_);
            return data_[i];
        }
        const char& operator[](std::size_t i) const
        {
            assert(i < size_);
            return data_[i];
        }

        buffer& operator=(buffer&& other)
        {
            if (this == &other)
                return *this;

            buffer tmp(std::move(*this));
            data_     = steal(other.data_);
            capacity_ = steal(other.capacity_);
            size_     = steal(other.size_);
            offset_   = steal(other.offset_);
            return *this;
        }

        bool empty() const
        {
            return size() == 0;
        }

        buffer clone() const
        {
            buffer clone((const void*)data_, size_);
            clone.offset_ = offset_;
            return clone;
        }

    private:
        template<typename T>
        T steal(T& loot) const
        {
            T val = loot;
            loot  = T();
            return val;
        }

        enum : std::uint32_t { CANARY= 0xcafebabe };

        char* data_;
        std::size_t capacity_; // current buffer capacity, i.e. amount of bytes pointed by data_
        std::size_t size_; // current total data size
        std::size_t offset_; // offset where the payload data begins.
    };

    inline
    std::size_t buffer_capacity(const buffer& buff)
    {
        return buff.size();
    }

    inline
    void* buffer_data(buffer& buff)
    {
        return &buff[0];
    }

    inline
    void grow_buffer(buffer& buff, size_t capacity)
    {
        buff.resize(capacity);
    }

    inline
    void trim_buffer(buffer& buff, size_t size)
    {
        buff.resize(size);
    }

    inline
    bool operator==(const buffer& lhs, const buffer& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        return !std::memcmp(&lhs[0], &rhs[0], lhs.size());
    }

} // newsflash
