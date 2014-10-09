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

#include <iterator>
#include <string>

namespace nntp
{
    // iterate over a buffer line by line
    class linebuffer
    {
    public:
        struct line {
            const char* start;
            size_t length; // length of the line including the \r\n (or \n)
        };

        class iterator : public
           std::iterator<std::forward_iterator_tag, line>
        {
        public:
            iterator(const linebuffer& buff, line current) : buffer_(&buff), current_(std::move(current))
            {
                pos_ = current_.length;
            }

            const line& operator * () const 
            {
                return current_;
            }
            const line* operator ->() const
            {
                return &current_;
            }

            // postfix
            iterator operator++(int)
            {
                iterator tmp(*this);
                forward();
                return tmp;
            }

            iterator& operator++()
            {
                forward();
                return *this;
            }

            bool operator==(const iterator& other) const
            {
                return other.current_.start == current_.start;
            }

            bool operator!=(const iterator& other) const
            {
                return !(other == *this);
            }

            std::string to_str() const
            {
                return std::string { current_.start, current_.length };
            }

        private:
            void forward()
            {
                current_ = buffer_->scan(pos_);
                pos_ += current_.length;
            }

        private:
            const linebuffer* buffer_;            
            line current_;
            std::size_t pos_;
        };

        linebuffer(const char* buffer, std::size_t size)
            : buffer_(buffer), size_(size)
        {}

        iterator begin() const
        {
            return iterator { *this, scan(0) };
        }

        iterator end() const
        {
            return iterator { *this, line{nullptr, 0}};
        }
    private:
        friend class iterator;

        line scan(std::size_t pos) const
        {
            if (size_ - pos < 2)
                return {nullptr, 0 };
            for (std::size_t i=pos; i<size_; ++i)
            {
                if (buffer_[i] == '\n')// && buffer_[i-1] == '\r')
                    return line {&buffer_[pos], i - pos + 1};
            }
            return line { nullptr, 0 };
        }

    private:
        const char* buffer_;
        const std::size_t size_;
    };


} // nntp