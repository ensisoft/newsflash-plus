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
#include "bitflag.h"

namespace newsflash
{
    template<typename StorageT, typename CacheT>
    class catalog : public StorageT, public CacheT
    {
    public:
        enum class flags : std::uint8_t {
            is_broken,
            is_binary,
            is_deleted,
            is_downloaded
        };

        struct article {
            bitflag<flags> flags;
            std::uint8_t len_subject;
            std::uint8_t len_author;
            const char* ptr_subject;
            const char* ptr_author;
        };

        catalog() : header_(nullptr), article_(nullptr)
        {}

        // open existing database for reading and writing
        void open()
        {
            header_ = (header*)StorageT::map_for_read_write(0, sizeof(header_));
            if (header_->cookie)
                throw std::runtime_error("missing header");
            if (header_->version != VERSION)
                throw std::runtime_error("incorrect version");
        }

        // create and begin a new database for reading and writing.
        void create()
        {
            header_ = (header*)StorageT::map_for_read_write(0, sizeof(header_));
            header_->cookie  = MAGIC;
            header_->version = VERSION;
            header_->fend    = sizeof(header);
            header_->num_articles = 0;
            std::memset(header_->table, 0, sizeof(header_->table));
        }

        // get article at the specified index.
        article get_article(std::size_t i)
        {
            assert(i < TSIZE);

            const auto offset = header_->table[i];

            assert(offset);
            article_ = StorageT::map_for_read(offset, 1024);

            auto* p = (char*)article_;

            article ret;

            ret.flags.set_from_value(*p++);
            ret.len_subject = *p++;
            ret.ptr_subject = p;
            p += ret.len_subject;
            ret.len_author = *p++;
            ret.ptr_author = p;
            return ret;
        }

        // insert article into the catalog at the specified index.
        void insert_article(std::size_t i, const article& a)
        {
            assert(i < TSIZE);

            auto offset = header_->table[i];
            if (offset == 0)
            {
                const auto len = 3 + a.len_author + a.len_subject;

                auto b = StorageT::map_for_write(header_->fend, 1024);
                auto p = (char*)b.ptr();
                *p++ = a.flags.value();
                *p++ = a.len_subject;
                std::memcpy(p, a.ptr_subject, a.len_subject);
                *p++ = a.len_author;
                std::memcpy(p, a.ptr_author, a.len_author);

                header_->num_articles++;
                header_->fend += len;
            }
            else 
            {
                auto b = StorageT::map_for_read_write(offset, 1024);
                auto p = (char*)b.ptr();

                bitflag<flags> f(*p);

                if (f.test(flags::is_deleted))
                {

                }
                else
                {

                }
            }
        }

        void set_article_flags(std::size_t i, bitflag<flags> f)
        {

        }

        bool has_article(std::size_t i) const 
        {
            assert(i < TSIZE);
            return header_->table[i] != 0;
        }

        std::uint32_t num_articles() const 
        {
            return header_->num_articles;
        }

    private:
        static const std::uint32_t MAGIC {0xdeadbabe};
        static const std::uint32_t TSIZE {1048576}; // 2^20 items
        static const std::uint32_t VERSION {1};

        struct header {
            std::uint32_t cookie;
            std::uint32_t version;
            std::uint32_t num_articles;
            std::uint32_t fend;
            std::uint32_t table[TSIZE];
        };
        header* header_;
        void* article_;
    };
} // newsflash