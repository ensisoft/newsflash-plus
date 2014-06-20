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

#include <newsflash/config.h>

#include <iterator>

#include "types.h"

#pragma once

namespace sdk
{
    // media category/type. each item is traditionally
    // tagged into a single category. 
    // int = international
    // sd  = standard definition
    // hd  = high definition
    enum class category {
        none            = 0,
        //console,
        console_nds     = (1 << 1),
        console_wii     = (1 << 2),
        console_xbox    = (1<< 3),
        console_xbox360 = (1 << 4),
        console_psp     = (1<< 5),
        console_ps2     = (1 << 6),
        console_ps3     = (1 << 7),
        console_ps4     = (1 << 8),
        console = (console_nds | console_wii | console_xbox | console_xbox360 | 
            console_psp | console_ps2 | console_ps3 | console_ps4),
        console_nintendo = (console_nds | console_wii),
        console_playstation = (console_psp | console_ps2 | console_ps3 | console_ps3 | console_ps4),
        console_microsoft = (console_xbox | console_xbox360),        

        //movies,
        movies_int      = (1 << 9),
        movies_sd       = (1 << 10),
        movies_hd       = (1 << 11),
        movies = (movies_int | movies_sd | movies_hd),
        
        //audio,
        audio_mp3       = (1 << 12),
        audio_video     = (1 << 13),
        audio_audiobook = (1 << 14),
        audio_lossless  = (1 << 15),
        audio = (audio_mp3 | audio_video | audio_audiobook | audio_lossless),

        //apps,
        apps_pc         = (1 << 16),
        apps_iso        = (1 << 17),
        apps_mac        = (1 << 18),
        apps_android    = (1 << 19),
        apps_ios        = (1 << 20),
        apps = (apps_pc | apps_ios | apps_mac | apps_android | apps_ios),

        //tv,
        tv_int          = (1 << 21),
        tv_sd           = (1 << 22),
        tv_hd           = (1 << 23),
        tv_other        = (1<< 24),
        tv_sport        = (1 << 25),
        tv = (tv_int | tv_sd | tv_hd | tv_other | tv_sport),

        //xxx,
        xxx_dvd         = (1<< 26),
        xxx_hd          = (1 << 27),
        xxx_sd          = (1 << 28),
        xxx = (xxx_dvd | xxx_hd | xxx_sd),

        //other,
        ebook = ( 1 << 29),
        other = (ebook),

        // sentinel
        last = (1 << 30),

        all = (console | movies | audio | apps | tv | xxx | other)
    };

    inline 
    bitflag_t operator | (category lhs, category rhs) 
    {
        return bitflag_t(lhs) | bitflag_t(rhs);
    }
    inline
    bitflag_t operator | (bitflag_t lhs, category rhs) 
    {
        return lhs | bitflag_t(rhs);
    }
    inline
    bitflag_t operator | (category lhs, bitflag_t rhs) 
    {
        return bitflag_t(lhs) | rhs;
    }

    inline
    bitflag_t operator & (category lhs, category rhs) 
    {
        return bitflag_t(lhs) & bitflag_t(rhs);
    }
    inline
    bitflag_t operator & (bitflag_t lhs, category rhs) 
    {
        return lhs & bitflag_t(rhs);
    }
    inline
    bitflag_t operator & (category lhs, bitflag_t rhs) 
    {
        return bitflag_t(lhs) & rhs;
    }

    class category_iterator : public std::iterator<std::forward_iterator_tag, sdk::category>
    {
    public:
        category_iterator(category beg) : cur_(beg)
        {}
        category_iterator() : cur_(category::last)
        {}

        // postfix
        category_iterator operator ++(int)
        {
            category_iterator tmp(cur_);
            auto value = BITFLAG(cur_);
            value <<= 1;
            cur_ = static_cast<category>(value);

            return tmp;
        }

        category_iterator& operator++()
        {
            auto value = BITFLAG(cur_);
            value <<= 1;
            cur_ = static_cast<category>(value);
            return *this;
        }
        category operator*() const 
        {
            return cur_;
        }
        category& operator*()
        {
            return cur_;
        }
        static 
        category_iterator begin() 
        {
            return category_iterator(category::console_nds);
        }
        static
        category_iterator end() 
        {
            return category_iterator(category::last);
        }

    private:
        friend bool operator==(const category_iterator&, const category_iterator&);
    private:
        category cur_;
    };

    inline
    bool operator==(const category_iterator& lhs, const category_iterator& rhs)
    {
        return lhs.cur_ == rhs.cur_;
    }
    inline 
    bool operator!=(const category_iterator& lhs, const category_iterator& rhs)
    {
        return !(lhs == rhs);
    }


    const char* str(category m);

} // sdk
