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
#include <limits>
#include <string>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <ctime>
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

        class iterator : public 
            std::iterator<std::input_iterator_tag, article<Storage>>
        {
        public:
            iterator() : offset_(0), length_(0)
            {}

            const article<Storage>& operator*() 
            {
                if (length_ == 0) {
                    article_ = catalog_->load(offset_t{offset_});
                    length_  = article_.size_on_disk();
                }
                return article_;
            }
            const article<Storage>* operator->() 
            {
                if (length_ == 0) {
                    article_ = catalog_->load(offset_t{offset_});
                    length_  = article_.size_on_disk();
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
                    article_ = catalog_->load(offset_t{offset_});
                    length_ = article_.size_on_disk();
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
            article<Storage> article_;
        };
 
        catalog()
        {
            header_.cookie  = MAGIC;
            header_.version = VERSION;
            header_.offset  = sizeof(header_);
            header_.size    = 0;
            header_.last    = std::numeric_limits<std::time_t>::min();
            header_.first   = std::numeric_limits<std::time_t>::max();
            std::memset(header_.table, 0, sizeof(header_.table));            
        }

        // open existing catalog for reading and writing
        // or create a new one if it doesn't yet exist.
        void open(std::string file)
        {
            Storage::open(file);
            if (Storage::size())
            {
                const auto buff = Storage::load(0, sizeof(header_),Storage::buf_read);
                std::copy(buff.begin(), buff.end(), (typename Storage::byte*)&header_);

                if (header_.cookie != MAGIC)
                    throw std::runtime_error("incorrect catalog header");
                if (header_.version != VERSION)
                    throw std::runtime_error("incorrect catalog version");
            }
        }

        iterator begin(offset_t offset) 
        {
            return {offset.value, this};
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
        article<Storage> load(offset_t offset)
        {
            ASSERT(offset.value < header_.offset);

            const auto off = offset.value + sizeof(header_);

            article<Storage> a;
            a.load(off, *this);
            return a;
        }

        // get the article at the specific index.
        article<Storage> load(index_t index)
        {
            ASSERT(index.value < CATALOG_SIZE);

            const auto off = header_.table[index.value];

            article<Storage> a;
            a.load(off, *this);
            return a;
        }

        // append an article into the catalog.      
        void append(const article<Storage>& a)
        {            
            ASSERT(header_.size < CATALOG_SIZE);

            const auto off = header_.offset;
            const auto idx = header_.size;

            a.save(off, *this);

            header_.table[idx] = off;
            header_.size++;
            header_.offset += a.size_on_disk();
            header_.first = std::min(a.pubdate(), header_.first);
            header_.last  = std::max(a.pubdate(), header_.last);
        }

        // insert article at the specified index.
        void insert(const article<Storage>& a, index_t i)
        {
            ASSERT(i.value < CATALOG_SIZE);
            ASSERT(header_.table[i.value] == 0);

            a.save(header_.offset, *this);
            
            header_.table[i.value] = header_.offset;
            header_.offset += a.size_on_disk();
            header_.size++;
            header_.first = std::min(a.pubdate(), header_.first);
            header_.last  = std::max(a.pubdate(), header_.last);            
            assert(header_.size < CATALOG_SIZE);
        }

        // update the article at the specified index.
        void update(const article<Storage>& a, index_t i)
        {
            ASSERT(i.value < CATALOG_SIZE);
            ASSERT(header_.table[i.value] != 0);
            a.save(header_.table[i.value], *this);
        }

        void flush()
        {
            auto buff = Storage::load(0, sizeof(header_), Storage::buf_write);
            auto beg  = (const typename Storage::byte*)&header_;
            auto end  = beg + sizeof(header_);
            std::copy(beg, end, buff.begin());
            buff.flush();
        }

        std::uint32_t size() const 
        {
            return header_.size;
        }

        std::time_t first_date() const 
        {
            return header_.first;
        }

        std::time_t last_date() const 
        {
            return header_.last;
        }

        bool is_empty(index_t i) const 
        {
            ASSERT(i.value < CATALOG_SIZE);
            return header_.table[i.value] == 0;
        }

    private:
        static const std::uint32_t MAGIC   {0xdeadbabe};
        static const std::uint32_t VERSION {2};

        struct header {
            std::uint32_t cookie;
            std::uint32_t version;
            std::uint32_t offset;
            std::uint32_t size;
            std::time_t   first;
            std::time_t   last;
            std::uint32_t table[CATALOG_SIZE];
        };
        header header_;
    };
} // newsflash
