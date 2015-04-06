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
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <ctime>
#include "nntp.h"
#include "assert.h"
#include "article.h"

namespace newsflash
{
    enum {
        CATALOG_SIZE = 100000
    };


    template<typename Storage>
    class catalog : public Storage
    {
    public:
        struct offset_t {
            offset_t(std::size_t off) : value(off)
            {}
            void operator+= (std::size_t val)
            {
                value += val;
            }

            std::size_t value;
        };

        struct index_t {
            index_t(std::size_t i) : value(i)
            {}
            std::size_t value;
        };
        using size_t = std::uint32_t;

        class iterator : public 
            std::iterator<std::input_iterator_tag, article>
        {
        public:
            iterator() : offset_(0), length_(0)
            {}

            const article& operator*() 
            {
                if (length_ == 0) {
                    article_ = catalog_->lookup(offset_t{offset_});
                    length_  = article_.length();
                }
                return article_;
            }
            const article* operator->() 
            {
                if (length_ == 0) {
                    article_ = catalog_->lookup(offset_t{offset_});
                    length_  = article_.length();
                }
                return &article_;
            }
            bool operator!=(const iterator& other) const 
            {
                return offset_ != other.offset_;
            }
            bool operator==(const iterator& other) const 
            {
                return offset_ == other.offset_;
            }
            iterator& operator++()
            {
                if (length_ == 0)
                {
                    article_ = catalog_->lookup(offset_t{offset_});
                    length_ = article_.length();
                }
                offset_ += length_;
                length_  = 0;
                return *this;
            }
            iterator operator++(int)
            {
                iterator i(*this);
                ++(*this);
                return i;
            }
            std::size_t offset() const 
            { return offset_; }
        private:
            friend class catalog;
            iterator(std::size_t offset, catalog* catalog) 
               : offset_(offset), length_(0), catalog_(catalog)
            {}

        private:
            std::size_t offset_;
            std::size_t length_;
            catalog* catalog_;
            article  article_;
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
            {
                auto buff = Storage::load(0, sizeof(header_),
                    Storage::buf_write);
                const auto* beg = (const typename Storage::byte*)(&header_);
                const auto* end = beg + sizeof(header_);
                std::copy(beg, end, buff.begin());
                return;
            }

            auto buff = Storage::load(0, sizeof(header_),
                Storage::buf_read | Storage::buf_write);
            std::copy(buff.begin(), buff.end(), (typename Storage::byte*)&header_);

            if (header_.cookie != MAGIC)
                throw std::runtime_error("incorrect catalog header");
            if (header_.version != VERSION)
                throw std::runtime_error("incorrect catalog version");
        }

        iterator begin()
        {
            return {0, this};
        }
        iterator end()
        {
            return {header_.offset - sizeof(header_), this};
        }

        // get article at the specific offset in the file.
        // the first article is at offset 0. 
        // In general article N+1 is at N.offset + N.length
        article lookup(offset_t offset)
        {
            const auto off = offset.value + sizeof(header_);
            return lookup(off);
        }

        // get the article at the specific index.
        article lookup(index_t index)
        {
            ASSERT(index.value < CATALOG_SIZE);
            const auto off = header_.table[index.value];
            return lookup(off);
        }

        // append an article into the catalog.      
        void append(const article& a)
        {
            const auto index = header_.article_count;
            ASSERT(index < CATALOG_SIZE);
            insert(index, a);
        }

        // insert article at the specified index.
        void insert(const article& a, index_t i)
        {
            ASSERT(i.value < CATALOG_SIZE);
            insert(i.value, a);
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
            return header_.table[i.value] == 0;
        }

    private:
        typedef typename Storage::buffer buffer;
        typedef typename Storage::buffer::iterator buffer_iterator;

        template<typename Value>
        void read(buffer_iterator& it, Value& val) const 
        {
            char* p = (char*)&val;
            for (std::size_t i=0; i<sizeof(val); ++i)
                p[i] = *it++; 
        }
        template<typename Value>
        void write(buffer_iterator& it, const Value& val) const 
        {
            const char* p = (const char*)&val;
            for (std::size_t i=0; i<sizeof(val); ++i)
                *it++ = p[i];
        }

        void read(buffer_iterator& it, std::string& str) const 
        {
            //const auto len = *it++;
            std::uint16_t len;
            read(it, len);
            const auto ptr = (char*)&(*it);
            it += len;
            str = std::string(ptr, len);
        }
        void write(buffer_iterator& it, const std::string& str) const 
        {
            const std::uint16_t len = str.size();
            //*it++ = str.size();
            write(it, len);
            std::copy(std::begin(str), std::end(str), it);
            it += str.size();
        }


        void read(buffer_iterator& it, bitflag<article::flags>& flags) const 
        {
            flags.set_from_value(*it++);
        }
        void write(buffer_iterator& it, const bitflag<article::flags>& flags) const 
        {
            *it++ = flags.value();
        }

        void insert(std::uint32_t index, const article& a)
        {
            const auto length = a.length();
            const auto empty  = header_.table[index] == 0;            
            const auto offset = empty ? header_.offset : header_.table[index];

        #ifdef NEWSFLASH_DEBUG
            if (!empty) {
                const auto& current = lookup(offset);
                assert(current.length() == a.length());
            }
        #endif

            auto buff = Storage::load(offset, length, Storage::buf_write);
            auto it = buff.begin();

            if (header_.article_start == 0)
                header_.article_start = a.number;

            std::int32_t number = 0;
            if (header_.article_start > a.number)
                 number = -(header_.article_start - a.number);
            else number = (header_.article_start - a.number);

            std::uint32_t magic = 0xc0febabe;

            write(it, a.bits);
            write(it, number);
            write(it, index);
            write(it, a.pubdate);
            write(it, a.bytes);
            write(it, a.partno);
            write(it, a.partmax);
            write(it, a.subject);
            write(it, a.author);
            write(it, magic);
            buff.flush();

            if (empty)
            {
                header_.table[index] = offset; //((length & 0xff) << 24) | offset;
                header_.offset += length;                
                header_.article_count++;
            }
        }
        article lookup(std::uint32_t offset) 
        {
            ASSERT(offset >= sizeof(header_));
            ASSERT(offset < header_.offset);
            const auto size = Storage::size();
            const auto min  = std::min(std::uint32_t(size - offset), std::uint32_t(1024));
            lookup_ = Storage::load(offset, min, Storage::buf_read);

            auto it = lookup_.begin();

            std::int32_t number = 0;
            std::uint32_t magic  = 0;

            article ret;
            read(it, ret.bits);
            read(it, number);
            read(it, ret.index);
            read(it, ret.pubdate);
            read(it, ret.bytes);
            read(it, ret.partno);
            read(it, ret.partmax);
            read(it, ret.subject);
            read(it, ret.author);
            read(it, magic);
            ASSERT(magic == 0xc0febabe);
            ret.number = header_.article_start + number;       
            ret.offset = offset - sizeof(header_);     
            return ret;            
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
            std::uint32_t table[CATALOG_SIZE];
        };
        header header_;
        buffer lookup_;
    };
} // newsflash
