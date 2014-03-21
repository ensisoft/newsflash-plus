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

#include <cstdint>
#include "types.h"

namespace corelib
{
    // index for the catalog file data access.
    class index
    {
    public:
        // type of the index
        enum class type {
            basic                     = 0x1,    // basic data only
            basic_and_author          = 0x2,    // basic data + author
            basic_and_date            = 0x3,    // basic data + date
            basic_and_author_and_date = 0x4     // basic data + author + date
        };

        // bit flags for the the index items. 
        enum class flags {
            is_broken         = 0x1,            // article is broken (not all parts are available)
            is_downloaded     = 0x2,            // article is downloaded
            is_binary         = 0x4,
            is_new            = 0x8,
            is_bookmarked     = 0x10,
            is_client_special = 0x20,
            is_dirty          = 0x40,
            is_deleted        = 0x80,
            is_regexmatch     = 0x100,
            is_filtermatch    = 0x200,
            was_regexmatch    = 0x400,
            was_filtermatch   = 0x800
        };

        struct item {
            std::uint32_t offset; // offset for the data in the catalog
            std::uint32_t partindex; // index into the article number array
            std::uint16_t partcount; // count of article numbers 
            std::uint16_t partmax; // is this needed?
            std::uint32_t bytes; // size of item in bytes
            std::uint32_t subject; // index for this item in the index when sorted by subject string
            std::uint32_t flags; // bitflags
        };

    private:

    };
} // corelib
