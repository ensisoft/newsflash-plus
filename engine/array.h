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
#include <cstdint>
#include <cstring>
#include <string>
#include "assert.h"

namespace newsflash
{
    // an array backed up by a specific Storage engine
    template<typename T, typename Storage>
    class array : public Storage
    {
    public:
        class proxy 
        {
        public:
            operator T()
            {
                return *(T*)buff_.address();
            }
            proxy& operator=(T value)
            {
                std::memcpy(buff_.address(), &value, sizeof(T));
                buff_.flush();
                return *this;
            }
        private:
            friend class array;
            proxy(typename Storage::buffer b) : buff_(std::move(b))
            {}
        private:
            typename Storage::buffer buff_;
        };

        array()
        {
            header_.size = 0;
        }

        void resize(std::size_t s)
        {
            header_.size = s;
            auto buff = Storage::load(0, sizeof(header_), Storage::buf_write);
            std::memcpy(buff.address(), &header_, sizeof(header_));
            buff.flush();
        }

        void open(std::string file)
        {
            Storage::open(file);
            if (Storage::size())
            {
                const auto buff = Storage::load(0, sizeof(header_), Storage::buf_read);
                std::memcpy(&header_, buff.address(), sizeof(header_));
                return;                
            }
        }
        proxy operator[](std::size_t i)
        {
            ASSERT(i < header_.size);
            const auto offset = sizeof(header_) + i * sizeof(T);
            auto buff = Storage::load(offset, sizeof(T), Storage::buf_read | Storage::buf_write);
            return {std::move(buff)};
        }

        std::size_t size() const 
        {
            return header_.size;
        }
    private:
        struct header {
            std::size_t size;
        };
        header header_;
    };
} // newsflash