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
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstring>
#include "utility.h"

namespace newsflash
{
    // NNTP data buffer. the buffer contents are split into 2 segments
    // the payload (body) and the response line that preceeds the data
    class buffer
    {
    public:
        enum class type {
            none, overview, article, grouplist, groupinfo
        };

        enum class status {
            none, success, unavailable, dmca, error
        };

        using u8 = char;

        buffer(std::size_t initial_capacity) : size_(0), content_start_(0), content_length_(0),
            content_type_(type::none), content_status_(status::none)
        {
            buffer_.resize(initial_capacity);
        }

        buffer() : size_(0), content_start_(0), content_length_(0),
            content_type_(type::none), content_status_(status::none)
        {}

        buffer(buffer&& other) : buffer_(std::move(other.buffer_))
        {
            size_           = other.size_;
            content_start_  = other.content_start_;
            content_length_ = other.content_length_;
            content_type_   = other.content_type_;
            content_status_ = other.content_status_;
        }

        // return content pointer to the start of the body/payload data
        const u8* content() const
        { return &buffer_[content_start_]; }

        // return head pointer to the start of the whole buffer
        const u8* head() const
        { return &buffer_[0]; }

        // return back pointer for writing data 
        u8* back() 
        { return &buffer_[size_]; }

        // after writing to the back() pointer, commit the number
        // of bytes written
        void append(std::size_t size)
        {
            assert(size + size_  <= buffer_.size());
            size_ += size;
        }

        void append(const char* str)
        {
            assert(std::strlen(str) + size_ <= buffer_.size());
            std::strcpy(back(), str);
            size_ += std::strlen(str);
        }

        void allocate(std::size_t capacity)
        {
            if (capacity < buffer_.size())
                return;

            buffer_.resize(capacity);
        }

        void clear()
        {
            size_ = 0;
            content_start_  = 0;
            content_length_ = 0;
            content_type_   = buffer::type::none;
            content_status_ = buffer::status::none;
        }

        // split the buffer into two buffers at the specified splitpoint.
        // the size, capacity and the contents of the split buffer are those
        // of this buffer from 0 to splitpoint.
        // after the split the contents of this buffer are shifted to 0 
        // and current size is decreased by splitpoint bytes.
        buffer split(std::size_t splitpoint)
        {
            assert(splitpoint <= size_);

            const auto bytes_to_copy = splitpoint;
            const auto bytes_to_move = size_ - splitpoint;

            buffer ret(bytes_to_copy);

            std::memcpy(&ret.buffer_[0],&buffer_[0], bytes_to_copy);
            std::memmove(&buffer_[0], &buffer_[splitpoint], bytes_to_move);

            ret.size_ = bytes_to_copy;
            size_ = bytes_to_move;
            return ret;
        }

        // set the buffer status
        void set_status(buffer::status status)
        { content_status_ = status; }

        // set content type
        void set_content_type(buffer::type type)
        { content_type_ = type;}


        // the buffer can contain both the simple nntp response header 
        // and the actual data content. instead of removing the header
        // we just specificy a start offset for the content to begin in the buffer
        void set_content_start(std::size_t start)
        { 
            assert(start + content_length_ <= size_);
            content_start_ = start; 
        }

        // set the actual length of the content data.
        void set_content_length(std::size_t length)
        { 
            assert(content_start_ + length <= size_);
            content_length_ = length; 
        }

        type content_type() const 
        { return content_type_; }

        status content_status() const
        { return content_status_; }

        // return the size of the whole buffer
        std::size_t size() const 
        { return size_; }

        std::size_t content_length() const
        { return content_length_; }

        std::size_t content_start() const 
        { return content_start_; }

        // return how many bytes are available for appending through back() pointer
        std::size_t available() const 
        { return buffer_.size() - size_; }

        bool full() const
        { return available() == 0; }


        buffer& operator=(buffer&& other)
        {
            buffer tmp(std::move(*this));
            
            buffer_         = std::move(other.buffer_);
            size_           = other.size_;
            content_start_  = other.content_start_;
            content_length_ = other.content_length_;
            content_type_   = other.content_type_;
            content_status_ = other.content_status_;
            return *this;
        }

    private:
        std::vector<u8> buffer_;
        std::size_t size_;
        std::size_t content_start_;
        std::size_t content_length_;
    private:
        type content_type_;
        status content_status_;
    };
} // newsflash
