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
#include "utility.h"

namespace newsflash
{
    // NNTP data buffer. the buffer contents are split into 2 segments
    // the payload (body) and the response line that preceeds the data
    class buffer
    {
    public:
        using u8 = char;

        buffer(std::size_t initial_capacity) : buffer_(initial_capacity), used_(0), header_length_(0), body_length_(0)
        {}

        // increase buffer size by some amount
        void increase()
        {
            const auto size = buffer_.size();
            if (size * 2 > MB(5))
                throw std::runtime_error("buffer capacity exceeded");

            buffer_.resize(size * 2);
        }

        const u8* head() const
        { return &buffer_[0]; }

        // return back pointer for writing data 
        u8* back() 
        { return &buffer_[used_]; }

        // after writing to the back() pointer, commit the number
        // of bytes written
        void commit(std::size_t num_bytes)
        {
            assert(used_ + num_bytes < buffer_.size());
            used_ += num_bytes;
        }

        // buffer split()
        // {

        // }

        // 
        void set_header_length(std::size_t length)
        { header_length_ = length; }

        void set_body_length(std::size_t length)
        { body_length_ = length; }

        // return the size of the whole buffer
        std::size_t size() const 
        { return used_; }

        // return how many bytes are available for appending through back() pointer
        std::size_t available() const 
        { return buffer_.size() - used_; }

    private:
        std::vector<u8> buffer_;
        std::size_t used_;
        std::size_t header_length_;
        std::size_t body_length_;
    };
} // newsflash
