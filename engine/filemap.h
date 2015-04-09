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
#include <iterator>
#include <memory>
#include <string>
#include "assert.h"

namespace newsflash
{
    // filemap maps the contents of a file on the disk into system ram in variable size chunks. 
    class filemap
    {
        class mapper;

    public:
        using byte = unsigned char;

        class buffer 
        {
        public:
        #ifdef NEWSFLASH_DEBUG
           template<typename T> 
           class iterator_t : public std::iterator<std::random_access_iterator_tag, T>
           {
           public:
               ~iterator_t()
                {}

                iterator_t()
                {}

                T& operator*() 
                {
                    ASSERT(pos_ < len_);
                    return base_[pos_];
                }
                T operator->()
                {
                    ASSERT(pos_ < len_);
                    return base_[pos_];
                }

                // todo: more ops here

                // postfix
                iterator_t operator++(int)
                {
                    iterator_t it(*this);
                    ++pos_;
                    return it;
                }

                iterator_t operator--(int)
                {
                    iterator_t it(*this);
                    --pos_;
                    return it;
                }

                iterator_t& operator++()
                {
                    ++pos_;
                    return *this;
                }
                iterator_t& operator--()
                {
                    --pos_;
                    return *this;
                }

                iterator_t& operator+=(int value)
                {
                    pos_ += value;
                    return *this;
                }
                iterator_t& operator-=(int value)
                {
                    pos_ -= value;
                    return *this;
                }

                iterator_t operator + (int n) const
                {
                    iterator_t i(*this);
                    i.len_ += n;
                    return i;
                }
                iterator_t operator - (int n) const
                {
                    iterator_t i(*this);
                    i.pos_ -= n;
                    return i;
                }

                std::size_t operator - (const iterator_t& other) const 
                {
                    return pos_ - other.pos_;
                }

                bool operator==(const iterator_t& other) const 
                {
                    return pos_ == other.pos_;
                }
                bool operator!=(const iterator_t& other) const
                {
                    return pos_ != other.pos_;
                }
                bool operator<(const iterator_t& other) const 
                {
                    return pos_ < other.pos_;
                }
                bool operator<=(const iterator_t& other) const 
                {
                    return pos_ <= other.pos_;
                }
                bool operator>(const iterator_t& other) const 
                {
                    return pos_ > other.pos_;
                }
                bool operator>=(const iterator_t& other) const
                {
                    return pos_ >= other.pos_;
                }

           private:
                iterator_t(byte* p, std::size_t len, std::size_t pos) : base_(p), len_(len), pos_(pos)
                {}
                friend class buffer;
            private:
                byte* base_;
            private:
                std::size_t len_;                
                std::size_t pos_;
           };

           using iterator = iterator_t<byte>;
           using const_iterator = iterator_t<const byte>;

           iterator begin()
           {
               return {(byte*)base_ + offset_, length_, 0};
           }
           iterator end()
           {
               return {(byte*)base_ + offset_, length_, length_ };
           }

           const_iterator begin() const 
           {
               return {(byte*)base_ + offset_, length_, 0};
           }
           const_iterator end() const 
           {
               return {(byte*)base_ + offset_, length_, length_};
           }

        #else
           using iterator = byte*;
           using const_iterator = const byte*;

           iterator begin() 
           {
                return (iterator)base_ + offset_;
           }
           iterator end() 
           {
                return begin() + length_;
           }

           const_iterator begin() const
           {
                return (const_iterator)base_ + offset_;
           }
           const_iterator end() const
           {
                return begin() + length_;
           }
        #endif

           buffer() : length_(0), offset_(0), base_(nullptr)
           {}

           buffer(buffer&& other) : mapper_(other.mapper_), length_(other.length_), offset_(other.offset_), base_(other.base_)
           {
               other.base_ = nullptr;
           }

           buffer(const buffer&) = delete;

          ~buffer();

           void* address()
           {
               return (byte*)base_ + offset_;; 
           }

           const void* address() const 
           {
               return (const byte*)base_ + offset_;
           }

           std::size_t length() const 
           { 
               return length_; 
           }

           buffer& operator=(const buffer&) = delete;

           buffer& operator=(buffer&& other) 
           {
                buffer carcass(std::move(*this));

                mapper_ = other.mapper_;
                length_ = other.length_;
                offset_ = other.offset_;
                base_   = other.base_;
                other.base_ = nullptr;
                return *this;
           }

           void flush()
           { /* this is a no-op */ }

        private:
            buffer(std::shared_ptr<mapper> m, std::size_t len, std::size_t off, void* b) : 
                mapper_(m), length_(len), offset_(off), base_(b)
            {}
            friend class filemap;
        private:
            std::shared_ptr<mapper> mapper_;
            std::size_t length_;
            std::size_t offset_;
        private:
            void* base_;
        };

        filemap();
       ~filemap();

        // open a filemap to the given file.
        void open(std::string file);

        enum buffer_flags {
            buf_read  = 1 << 0,
            buf_write = 1 << 1
        };

        // load a buffer of data from the file
        buffer load(std::size_t offset, std::size_t size, unsigned flags);

        std::size_t size() const;

        std::string filename() const;

    private:
        std::shared_ptr<mapper> mapper_;
        std::string filename_;
    };

} // newsflash
