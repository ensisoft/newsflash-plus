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
#include <ctime>
#include <string>
#include "bitflag.h"

namespace newsflash
{
    // catalog article. 
    struct article 
    {
        enum class flags : std::uint8_t {
            // the article is broken. i.e. not all parts are known.
            broken, 

            // the article appears to contain binary content.
            binary, 

            // the article is deleted.
            deleted,

            // the article is downloaded.
            downloaded,

            // 
            bookmarked
        };

        // set flags bits.
        bitflag<flags> bits;

        // offset in the catalog.
        std::uint32_t offset;    

        // index in the catalog.
        std::uint32_t index;

        //         
        std::uint64_t number;

        // the size of the article in bytes.
        std::uint32_t bytes;

        // publication data in ctime
        std::time_t   pubdate;

        // number of parts known.
        std::uint8_t  partno;

        // expected number of parts.
        std::uint8_t  partmax;

        std::string subject;

        std::string author;

        std::size_t length() const {
           return 21 + sizeof(std::time_t) + subject.size() + author.size();
        }
        std::uint32_t next() const {
           return offset + length();
        }
        bool empty() const {
           return subject.empty();
        }
    };

    inline 
    bool operator==(const article& lhs, const article& rhs)
    {
        return lhs.offset == rhs.offset;
    }

} // newsflash