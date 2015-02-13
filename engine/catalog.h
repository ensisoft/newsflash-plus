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
    template<typename Storage>
    class catalog : public Storage
    {
    public:
        enum class flags : std::uint8_t {
            is_broken,
            is_binary,
            is_deleted,
            is_downloaded
        };

        using key_t = std::size_t;

        struct article {
            bitflag<flags> state;
            std::uint8_t len_subject;
            std::uint8_t len_author;
            const char* ptr_subject;
            const char* ptr_author;

            std::size_t length() const {
                return 3 + len_subject + len_author;
            }
            key_t next() const {
                return length();
            }
        };

        catalog()
        {}

        // open existing catalog for reading and writing
        // or create a new one if it doesn't yet exist.
        void open(std::string file)
        {
            Storage::open(file);

            if (Storage::size())
            {
                auto buff = Storage::load(0, sizeof(header_),
                    Storage::buf_read | Storage::buf_write);
                std::copy(buff.begin(), buff.end(), (typename Storage::byte*)&header_);

                if (header_.cookie != MAGIC)
                    throw std::runtime_error("incorrect header");
                if (header_.version != VERSION)
                    throw std::runtime_error("incorrect version");
            }
            else
            {
                header_.cookie       = MAGIC;
                header_.version      = VERSION;
                header_.offset       = sizeof(header_);
                header_.num_articles = 0;
                header_.last_article_number = 0;
                header_.first_article_number = 0;
            }
        }



        // get article with the given key.
        article lookup(key_t key)
        {
            const auto off = sizeof(header_) + key;
            const auto end = header_.offset;
            ASSERT(off < end);

            lookup_ = Storage::load(off, 1024, Storage::buf_read);

            auto it = lookup_.begin();
            article ret;
            ret.state.set_from_value(*it++);
            ret.len_subject = *it++;
            ret.ptr_subject = (char*)&(*it);
            it += ret.len_subject;
            ret.len_author  = *it++;
            ret.ptr_author  = (char*)&(*it);
            it += ret.len_author;
            return ret;
        }

        // insert an article into the catalog.
        // returns a key that can be later used to retrieve the article
        key_t insert(const article& a)
        {
            const auto length = a.length();
            const auto offset = header_.offset;

            auto buff = Storage::load(offset, length, Storage::buf_write);
            auto it = buff.begin();
            *it++ = a.state.value();
            *it++ = a.len_subject;
            std::copy(a.ptr_subject, a.ptr_subject + a.len_subject, it);
            it += a.len_subject;
            *it++ = a.len_author;
            std::copy(a.ptr_author, a.ptr_author + a.len_author, it);

            buff.flush();

            header_.offset += length;
            header_.num_articles++;
            return offset - sizeof(header_);
        }

        void flush()
        {
            auto buff = Storage::load(0, sizeof(header_), Storage::buf_write);
            auto beg  = (const typename Storage::byte*)&header_;
            auto end  = beg + sizeof(header_);
            std::copy(beg, end, buff.begin());
            buff.flush();
        }

        std::uint32_t num_articles() const 
        {
            return header_.num_articles;
        }

        std::uint64_t last_article_number() const 
        {
            return header_.last_article_number;
        }
        std::uint64_t first_article_number() const
        {
            return header_.first_article_number;
        }

    private:
        static const std::uint32_t MAGIC   {0xdeadbabe};
        static const std::uint32_t VERSION {1};

        struct header {
            std::uint32_t cookie;
            std::uint32_t version;
            std::uint32_t offset;
            std::uint32_t num_articles;
            std::uint64_t last_article_number;
            std::uint64_t first_article_number;
        };
        header header_;

        typename Storage::buffer lookup_;
    };
} // newsflash