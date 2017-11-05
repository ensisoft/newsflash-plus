// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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
        CATALOG_SIZE = 0x20000
    };

    template<typename StorageDevice>
    class catalog
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
            std::iterator<std::input_iterator_tag, article<StorageDevice>>
        {
        public:
            iterator() : offset_(0), length_(0)
            {}

            const article<StorageDevice>& operator*()
            {
                if (length_ == 0) {
                    article_ = catalog_->load(offset_t{offset_});
                    length_  = article_.size_on_disk();
                }
                return article_;
            }
            const article<StorageDevice>* operator->()
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
            article<StorageDevice> article_;
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

        struct snapshot_t {
            std::uint32_t offset = 0;
            std::uint32_t size   = 0;
            std::uint32_t first  = 0;
            std::uint32_t last   = 0;
            std::uint32_t table[CATALOG_SIZE];
            std::string   file;
        };

        std::unique_ptr<snapshot_t> snapshot() const
        {
            auto ret = std::make_unique<snapshot_t>();
            ret->offset = header_.offset;
            ret->size   = header_.size;
            ret->first  = header_.first;
            ret->last   = header_.last;
            ret->file   = device_.filename();
            std::memcpy(ret->table, header_.table, sizeof(header_.table));
            return ret;
        }

        // open an existing catalog for reading based on a the given snapshot
        // the snapshot contains enough data that allows a consistent
        // view of the catalog to be opened.
        void open(const snapshot_t& snapshot)
        {
            device_.open(snapshot.file);
            header_.offset = snapshot.offset;
            header_.size   = snapshot.size;
            header_.first  = snapshot.first;
            header_.last   = snapshot.last;
            std::memcpy(header_.table, snapshot.table, sizeof(header_.table));
        }

        // open existing catalog for reading and writing
        // or create a new one if it doesn't yet exist.
        void open(const std::string& file)
        {
            device_.open(file);
            if (device_.size())
            {
                const auto buff = device_.load(0, sizeof(header_), StorageDevice::buf_read);
                std::copy(buff.begin(), buff.end(), (typename StorageDevice::byte*)&header_);

                if (header_.cookie != MAGIC)
                    throw std::runtime_error("incorrect catalog header");
                if (header_.version != VERSION)
                    throw std::runtime_error("incorrect catalog version");
            }
        }

        void close()
        {
            device_.close();
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
        article<StorageDevice> load(offset_t offset)
        {
            ASSERT(offset.value < header_.offset);

            const auto off = offset.value + sizeof(header_);

            article<StorageDevice> a;
            a.load(off, device_);
            return a;
        }

        // get the article at the specific index.
        article<StorageDevice> load(index_t index)
        {
            ASSERT(index.value < CATALOG_SIZE);

            const auto off = header_.table[index.value];

            article<StorageDevice> a;
            a.load(off, device_);
            return a;
        }

        // append an article into the catalog.
        void append(const article<StorageDevice>& a)
        {
            ASSERT(header_.size < CATALOG_SIZE);

            const auto off = header_.offset;
            const auto idx = header_.size;

            a.save(off, device_);

            header_.table[idx] = off;
            header_.size++;
            header_.offset += a.size_on_disk();
            header_.first = std::min(a.pubdate(), header_.first);
            header_.last  = std::max(a.pubdate(), header_.last);
        }

        // insert article at the specified index.
        void insert(const article<StorageDevice>& a, index_t i)
        {
            ASSERT(i.value < CATALOG_SIZE);
            ASSERT(header_.table[i.value] == 0);

            a.save(header_.offset, device_);

            header_.table[i.value] = header_.offset;
            header_.offset += a.size_on_disk();
            header_.size++;
            header_.first = std::min(a.pubdate(), header_.first);
            header_.last  = std::max(a.pubdate(), header_.last);
            ASSERT(header_.size <= CATALOG_SIZE);
        }

        // update the article at the specified index.
        void update(const article<StorageDevice>& a, index_t i)
        {
            ASSERT(i.value < CATALOG_SIZE);
            ASSERT(header_.table[i.value] != 0);
            a.save(header_.table[i.value], device_);
        }

        void flush()
        {
            auto buff = device_.load(0, sizeof(header_), StorageDevice::buf_write);
            auto beg  = (const typename StorageDevice::byte*)&header_;
            auto end  = beg + sizeof(header_);
            std::copy(beg, end, buff.begin());
            buff.write();
            device_.flush();
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

        const StorageDevice& device() const
        { return device_; }

    private:
        static const std::uint32_t MAGIC = 0xdeadbabe;
        // version 3. refactoring the way how to we store the data
        // in the data files. instead of using the article hash
        // to index into the catalog file we use a local article
        // number that is derived from the very first article number
        // received from the server + local article offset.
        //
        // version 4. bumping the version number because (stupidly)
        // the idlist didn't have a proper versioning schema of it's own.
        // so we're relying on the catalog version.
        // also the article's datatype for idlist key has been changed to 64bit
        //
        // version 5. change the catalog size to a power of 2.
        // this allows the the update operation to get rid of the modulo operator and replace it
        // with a bitwise and.
        static const std::uint32_t VERSION = 5;

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

        StorageDevice device_;
    };
} // newsflash
