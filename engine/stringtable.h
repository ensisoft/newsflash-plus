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

#include <newsflash/config.h>

#include <vector>
#include <string>
#include <map>
#include <cstddef>

namespace newsflash
{
    // string lookup table
    class stringtable 
    {
    public:
        typedef std::size_t key_t;

        static const std::size_t MAX_STRING_LEN = 256;

        stringtable();


        // add a string to the table.
        // the string should not exceed MAX_STRING_LENGTH. 
        // Strings longer than MAX_STRING_LEN will be truncated.
        key_t add(const char* str, std::size_t len);

        // get a string back from the table based on the
        // key value. key must have come from a prior call
        // to add function. 
        std::string get(key_t key) const;

    private:
        #pragma pack(push)
        #pragma pack(1)
        struct delta {
            std::uint8_t pos;
            std::uint8_t val;
        };

        // difference string w/r to the basestring
        struct diffstring {
            std::uint32_t offset;
            std::uint8_t  length;
            std::uint8_t  count;
            delta deltas[MAX_STRING_LEN];
        };

        static_assert(sizeof(diffstring) == 6 + sizeof(delta) * MAX_STRING_LEN
            , "string was expected to be packed");

        // basestring
        struct basestring {
            std::uint8_t  len; // length of the data to follow
            char data[MAX_STRING_LEN]; 
        };

        #pragma pack(pop)

        std::pair<basestring*, std::size_t> find_base_string(std::size_t bucket) const;
        std::pair<basestring*, std::size_t> insert_base_string(std::size_t bucket, const char* str, std::size_t len);

        const diffstring* load_diff_string(std::size_t offset) const;
        const basestring* load_base_string(std::size_t offset) const;

    private:

        std::vector<char> basedata_;
        std::vector<char> diffdata_;
        std::map<std::size_t, std::size_t> offsets_;
        std::size_t rawbytes_;
    };
} // newsflash
