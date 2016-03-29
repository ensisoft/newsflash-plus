// Copyright (c) 2010-2013 Sami Väisänen, Ensisoft
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

// $Id: utf8.h,v 1.8 2010/07/25 12:59:51 svaisane Exp $

#pragma once

#include <cstdint>
#include <cassert>
#include <cstring>
#include <string>
#include <iterator>

namespace utf8
{
    namespace detail {
        template<typename T>
        struct integer_type;

        template<>
        struct integer_type<int>
        {
            typedef unsigned int unsigned_type;
        };

        template<>
        struct integer_type<long>
        {
            typedef unsigned long unsigned_type;
        };

        template<>
        struct integer_type<unsigned long>
        {
            typedef unsigned long unsigned_type;
        };

        template<>
        struct integer_type<unsigned int>
        {
            typedef unsigned int unsigned_type;
        };

        template<>
        struct integer_type<char>
        {
            typedef unsigned char unsigned_type;
        };
        template<>
        struct integer_type<wchar_t>
        {
            //typedef unsigned wchar_t unsigned_type;
            typedef unsigned short unsigned_type;
        };
    };

    // encode the range specified by beg and end iterators
    // as an utf8 encoded byte stream into the output iterator dest.
    template<typename InputIterator, typename OutputIterator>
    void encode(InputIterator beg, InputIterator end, OutputIterator dest)
    {
        typedef typename std::iterator_traits<InputIterator>::value_type value_type;
        typedef typename detail::integer_type<value_type>::unsigned_type unsigned_type;

        // Unicode conversion table
        // number range (4 bytes)| binary representation (octets)
        // -----------------------------------------------------------
        // 0000 0000 - 0000 007F | 0xxxxxxx                 (US-ASCII)
        // 0000 0080 - 0000 07FF | 110xxxxx 10xxxxxx
        // 0000 0800 - 0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
        // 0001 0000 - 0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        while (beg != end)
        {
            // maximum size for a Unicode character is 4 bytes
            const uint32_t wchar = static_cast<unsigned_type>(*beg++);

            if (wchar <= 0x007F)
            {
                *dest++ = static_cast<char>(wchar);
            }
            else if (wchar >= 0x0080 &&  wchar <= 0x07FF)
            {
                *dest++ = static_cast<char>((wchar >> 6)   | 0xC0);
                *dest++ = static_cast<char>((wchar & 0x3F) | 0x80);
            }
            else if (wchar >= 0x0800 && wchar <= 0xFFFF)
            {
                *dest++ = static_cast<char>((wchar >> 12)  | 0xE0);
                *dest++ = static_cast<char>(((wchar >> 6)  & 0x3F) | 0x80);
                *dest++ = static_cast<char>((wchar & 0x3F) | 0x80);
            }
            else
            {
                *dest++ = static_cast<char>((wchar >> 18) | 0xF0);
                *dest++ = static_cast<char>(((wchar >> 12) & 0x3F) | 0x80);
                *dest++ = static_cast<char>(((wchar >>  6) & 0x3F) | 0x80);
                *dest++ = static_cast<char>((wchar & 0x3F) | 0x80);
            }
        } // while
    }

    // convenience function to encode (extended) ascii string to utf8
    inline std::string encode(const std::string& ascii)
    {
        std::string utf8;
        encode(ascii.begin(), ascii.end(),
            std::back_inserter(utf8));
        return utf8;
    }

    // convenience function to encode a wide unicode string to utf8
    inline std::string encode(const std::wstring& unicode)
    {
        std::string utf8;
        encode(unicode.begin(), unicode.end(),
            std::back_inserter(utf8));
        return utf8;
    }

    template<typename InputIterator>
    bool is_well_formed(InputIterator beg, InputIterator end)
    {
#define WELL_FORMED_RANGE(x, beg, end) \
        (x >= beg && x <= end)

#define NEXT_BYTE() \
    do { \
        if (++beg == end) \
            return false; \
        byte = *beg; \
    } while(0)

        while (beg != end)
        {
            auto byte = *beg;
            if (WELL_FORMED_RANGE(byte, 0x00, 0x7F)) {
                // single byte sequence
            }
            else if (WELL_FORMED_RANGE(byte, 0xC2, 0xDF)) {
                // two byte sequence
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x080, 0xBF))
                    return false;
            }
            else if (WELL_FORMED_RANGE(byte, 0xE0, 0xE0)) {
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0xA0, 0xBF))
                    return false;
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBF))
                    return false;
            }
            else if (WELL_FORMED_RANGE(byte, 0xE1, 0xEC)) {
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBF))
                    return false;
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBF))
                    return false;
            }
            else if (WELL_FORMED_RANGE(byte, 0xED, 0xED)) {
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0x9F))
                    return false;
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBF))
                    return false;
            }
            else if (WELL_FORMED_RANGE(byte, 0xEE, 0xEF)) {
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBF))
                    return false;
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBF))
                    return false;
            }
            else if (WELL_FORMED_RANGE(byte, 0xF0, 0xF0)) {
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x90, 0xBF))
                    return false;
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBf))
                    return false;
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBf))
                    return false;
            }
            else if (WELL_FORMED_RANGE(byte, 0xF1, 0xF3)) {
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBF))
                    return false;
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBf))
                    return false;
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBf))
                    return false;
            }
            else if (WELL_FORMED_RANGE(byte, 0xF4, 0xF4)) {
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBF))
                    return false;
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBf))
                    return false;
                NEXT_BYTE();
                if (!WELL_FORMED_RANGE(byte, 0x80, 0xBf))
                    return false;
            }
            else
            {
                return false;
            }
            ++beg;
        }
        return true;
    }


    template<typename WideChar, typename InputIterator, typename OutputIterator>
    InputIterator decode(InputIterator beg, InputIterator end, OutputIterator dest)
    {

        typedef WideChar wide_t;

#define READBITS(val_mask, shift, advance) \
        if (true) {\
            const std::uint8_t val = *beg;\
            const std::uint8_t key = ~((val_mask + 1) | val_mask);\
            const std::uint8_t key_mask = ~val_mask;\
            if ((key_mask & val) != key)\
                return pos;\
            wide |= wide_t(val_mask & val) << shift;\
            if (advance) { \
                if (++beg == end)\
                    return pos;\
            }\
        }

        InputIterator pos = beg;

        while (beg != end)
        {
            pos = beg;
            wide_t wide = 0;
            switch (*beg & 0xF0)
            {
                case 0xF0: // 4 byte sequence
                    assert(sizeof(wide_t) >= 4);

                    // invalid values would encode numbers larger than
                    // then 0x10ffff limit of Unicode.
                    if (*beg >= 0xF5)
                        return pos;

                    READBITS(0x07, 18, true);
                    READBITS(0x3f, 12, true);
                    READBITS(0x3f, 6, true);
                    READBITS(0x3f, 0, false);

                    if (wide >= 0x10ffff)
                        return pos;
                    *dest++ = wide;
                    break;

                case 0xE0: // 3 byte sequence (fits in 16 bits)
                    assert(sizeof(wide_t) >= 2);

                    READBITS(0x0f, 12, true);
                    READBITS(0x3f, 6, true);
                    READBITS(0x3f, 0, false);

                    *dest++ = wide;
                    break;

                case 0xC0: // 2 byte sequence
                case 0xD0:
                    assert(sizeof(wide_t) >= 2);

                    // invalid values. could only be used for
                    // "overlong encoding" of ASCII characters.
                    if (*beg == 0xC0 || *beg == 0xC1)
                        return pos;

                    READBITS(0x1f, 6, true);
                    READBITS(0x3f, 0, false);

                    *dest++ = wide;
                    break;

                    // continuation bytes should not appear by themselves
                    // but only after a leading byte
                case 0x80:
                case 0x90:
                case 0xA0:
                case 0xB0:
                    return pos;

                default:   // 1 byte sequence (ascii)
                    if (0x80 & *beg)
                        return pos;
                    *dest++ = *beg;
                    break;
            }
            ++beg;
        }
        return beg;

    #undef READBITS
    }

    inline
    std::wstring decode(const std::string& utf8, bool* success = nullptr)
    {
        std::wstring ret;
        const auto pos = decode<wchar_t>(utf8.begin(), utf8.end(), std::back_inserter(ret));

        if (success)
            *success = (pos == utf8.end());

        return ret;
    }

    inline
    std::wstring decode(const char* utf8, bool* success = nullptr)
    {
        std::wstring ret;
        const auto beg = utf8;
        const auto end = utf8 + std::strlen(utf8);
        const auto pos = decode<wchar_t>(beg, end, std::back_inserter(ret));

        if (success)
            *success = (pos == end);

        return ret;
    }

} // utf8



