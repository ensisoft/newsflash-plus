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
#include <stdexcept>
#include <string>
#include <cstdint>
#include <cassert>
#include <cstring>
#include "assert.h"
#include "bitflag.h"

namespace newsflash
{
    enum {
        CATALOG_SIZE = 100000
    };


    template<typename Storage>
    class catalog : public Storage
    {
    public:
        enum class flags : std::uint8_t {
            broken, binary, recent,
            deleted,
            downloaded,
            bookmarked
        };

        struct offset_t {
            offset_t(std::size_t off) : value(off)
            {}

            std::size_t value;
        };

        struct index_t {
            index_t(std::size_t i) : value(i)
            {}
            std::size_t value;
        };

        struct article {
            bitflag<flags> bits;
            std::uint64_t number;
            std::uint32_t bytes;
            std::uint8_t  parts;
            std::uint8_t  partmax;
            std::uint8_t  len_subject;
            std::uint8_t  len_author;                
            const char* ptr_subject;
            const char* ptr_author;

            std::size_t length() const {
                return 13 + len_subject + len_author;
            }
            offset_t next() const {
                return length();
            }
        };

        catalog()
        {
            header_.cookie        = MAGIC;
            header_.version       = VERSION;
            header_.offset        = sizeof(header_);
            header_.article_count = 0;
            header_.article_start = 0;
            std::memset(header_.table, 0, sizeof(header_.table));            
        }

        // open existing catalog for reading and writing
        // or create a new one if it doesn't yet exist.
        void open(std::string file)
        {
            Storage::open(file);
            if (Storage::size() == 0)
                return;

            auto buff = Storage::load(0, sizeof(header_),
                Storage::buf_read | Storage::buf_write);
            std::copy(buff.begin(), buff.end(), (typename Storage::byte*)&header_);

            if (header_.cookie != MAGIC)
                throw std::runtime_error("incorrect header");
            if (header_.version != VERSION)
                throw std::runtime_error("incorrect version");
        }

        // get article with the given key.
        article lookup(offset_t offset)
        {
            const auto off = sizeof(header_) + offset.value;
            const auto end = header_.offset;
            ASSERT(off < end);

            lookup_ = Storage::load(off, 1024, Storage::buf_read);

            auto it = lookup_.begin();

            std::int32_t number = 0;

            article ret;
            read(it, ret.bits);
            read(it, number);
            read(it, ret.bytes);
            read(it, ret.parts);
            read(it, ret.partmax);
            read(it, ret.ptr_subject, ret.len_subject);
            read(it, ret.ptr_author, ret.len_author);
            ret.number = header_.article_start + number;            
            return ret;
        }

        article lookup(index_t index)
        {
            ASSERT(index.value < CATALOG_SIZE);
            const auto off = header_.table[index.value];
            return lookup(offset_t{off});
        }

        // append an article into the catalog.
        // returns a key that can be later used to retrieve the article by offset
        void append(const article& a)
        {
            const auto index = header_.article_count;
            ASSERT(index < CATALOG_SIZE);
            insert(index, a);
        }

        void insert(const article& a, index_t i)
        {
            ASSERT(i.value < CATALOG_SIZE);
            insert(i.value, a);
        }

        void update(const article& a, index_t i)
        {
            ASSERT(i.value < CATALOG_SIZE);
            
        }

        void flush()
        {
            auto buff = Storage::load(0, sizeof(header_), Storage::buf_write);
            auto beg  = (const typename Storage::byte*)&header_;
            auto end  = beg + sizeof(header_);
            std::copy(beg, end, buff.begin());
            buff.flush();
        }

        std::uint32_t article_count() const 
        {
            return header_.article_count;
        }

        std::uint64_t article_start() const 
        {
            return header_.article_start;
        }

        bool is_empty(index_t i) const 
        {
            ASSERT(i.value < CATALOG_SIZE);
            return header_.table[i.value] != 0;
        }

    private:
        typedef typename Storage::buffer buffer;
        typedef typename Storage::buffer::iterator iterator;

        template<typename Value>
        void read(iterator& it, Value& val) const 
        {
            char* p = (char*)&val;
            for (std::size_t i=0; i<sizeof(val); ++i)
                p[i] = *it++; 
        }
        template<typename Value>
        void write(iterator& it, const Value& val) const 
        {
            const char* p = (const char*)&val;
            for (std::size_t i=0; i<sizeof(val); ++i)
                *it++ = p[i];
        }

        void read(iterator& it, bitflag<flags>& flags) const 
        {
            flags.set_from_value(*it++);
        }
        void write(iterator& it, const bitflag<flags>& flags) const 
        {
            *it++ = flags.value();
        }
        void read(iterator& it, const char*& ptr, std::uint8_t& len) const
        {
            len = *it++;
            ptr = (char*)&(*it);
            it += len;
        }
        void write(iterator& it, const char* ptr, std::uint8_t len) const 
        {
            *it++ = len;
            std::copy(ptr, ptr + len, it);
            it += len;
        }

        void insert(std::size_t index, const article& a)
        {
            const auto length = a.length();
            const auto offset = header_.offset;

            auto buff = Storage::load(offset, length, Storage::buf_write);
            auto it = buff.begin();

            if (header_.article_start == 0)
                header_.article_start = a.number;

            std::int32_t number = 0;
            if (header_.article_start > a.number)
                 number = -(header_.article_start - a.number);
            else number = (header_.article_start - a.number);

            write(it, a.bits);
            write(it, number);
            write(it, a.bytes);
            write(it, a.parts);
            write(it, a.partmax);
            write(it, a.ptr_subject, a.len_subject);
            write(it, a.ptr_author, a.len_author);
            buff.flush();

            header_.table[index] = offset;
            header_.offset += length;
            header_.article_count++;
        }

    private:
        static const std::uint32_t MAGIC   {0xdeadbabe};
        static const std::uint32_t VERSION {1};

        struct header {
            std::uint32_t cookie;
            std::uint32_t version;
            std::uint32_t offset;
            std::uint32_t article_count;
            std::uint64_t article_start;
            std::uint32_t table[2];
        };
        header header_;
        buffer lookup_;
    };
} // newsflash
