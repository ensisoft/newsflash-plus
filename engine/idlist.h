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

#include "newsflash/config.h"

#include <cstdint>
#include <cstring>
#include <string>

#include "assert.h"

namespace newsflash
{
    // an array message ids
    template<typename StorageDevice>
    class idlist
    {
    public:
        class proxy
        {
        public:
            operator std::int16_t()
            {
                return *(std::int16_t*)buff_.address();
            }
            proxy& operator=(std::int16_t value)
            {
                std::memcpy(buff_.address(), &value, sizeof(value));
                buff_.flush();
                return *this;
            }
        private:
            friend class idlist;
            proxy(typename StorageDevice::buffer b) : buff_(std::move(b))
            {}
        private:
            typename StorageDevice::buffer buff_;
        };

        void resize(std::uint64_t s)
        {
            header_.size = s;
            auto buff = device_.load(0, sizeof(header_), StorageDevice::buf_write);
            std::memcpy(buff.address(), &header_, sizeof(header_));
            buff.flush();
        }

        void open(const std::string& file)
        {
            device_.open(file);
            if (device_.size())
            {
                const auto buff = device_.load(0, sizeof(header_), StorageDevice::buf_read);
                std::memcpy(&header_, buff.address(), sizeof(header_));
                if (header_.cookie != COOKIE)
                    throw std::runtime_error("incorrect idlist header");
                if (header_.version != VERSION)
                    throw std::runtime_error("incorrect catalog version");
            }
        }
        proxy operator[](std::uint64_t i)
        {
            ASSERT(i < header_.size);
            const auto offset = sizeof(header_) + i * sizeof(std::int16_t);
            auto buff = device_.load(offset, sizeof(std::int16_t), StorageDevice::buf_read | StorageDevice::buf_write);
            return {std::move(buff)};
        }

        bool is_open() const 
        { return device_.is_open(); }

        std::uint32_t size() const
        { return header_.size; }

        std::uint32_t version() const
        { return header_.version; }
    private:
        // this has been some sloppy programming. the header lacks
        // a version field which makes it unfortunately hard to revise
        // this data structure. Initially the size has been of type size_t
        // but now with 32bit legacy we need to fix that to a uint32_t.
        // Ideally now that we have 64bit support (on windows also) the
        // type should be uint64_t but that creates a problem with the
        // existing data since we don't have the version information.
        // for now we're just going to fix the 32 vs. 64bit issue
        // by using a uint32_t and reconsider the versioning later if
        // there's a need to support more than 4b parts.

        // UPDATE: commit 2216e94 Improve the header file data usage
        // actually breaks the binary compatibility for the header files
        // but that commit failed to fix this issue.
        static const std::uint32_t COOKIE  = 0xFDFD0123;
        static const std::uint32_t VERSION = 1;

        struct header {
            std::uint32_t cookie  = COOKIE;
            std::uint32_t version = VERSION;
            std::uint64_t size    = 0;
        };
        header header_;

        StorageDevice device_;
    };
} // newsflash