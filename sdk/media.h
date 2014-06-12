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

#include "types.h"

#pragma once

namespace sdk
{
    // media category/type. each item is traditionally
    // tagged into a single category. 
    // int = international
    // sd  = standard definition
    // hd  = high definition
    enum class media {
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
        
        //movies,
        movies_int      = (1 << 9),
        movies_sd       = (1 << 10),
        movies_hd       = (1 << 11),
        
        //audio,
        audio_mp3       = (1 << 12),
        audio_video     = (1 << 13),
        audio_audiobook = (1 << 14),
        audio_lossless  = (1 << 15),
        
        //apps,
        apps_pc         = (1 << 16),
        apps_iso        = (1 << 17),
        apps_mac        = (1 << 18),
        apps_android    = (1 << 19),
        apps_ios        = (1 << 20),
        
        //tv,
        tv_int          = (1 << 21),
        tv_sd           = (1 << 22),
        tv_hd           = (1 << 23),
        tv_other        = (1<< 24),
        tv_sport        = (1 << 25),
        
        //xxx,
        xxx_dvd         = (1<< 26),
        xxx_hd          = (1 << 27),
        xxx_sd          = (1 << 28),
        
        //other,
        ebook           = ( 1 << 29)
    };

    inline 
    media operator | (media lhs, media rhs) {
        return static_cast<media>(bitflag_t(lhs) | bitflag_t(rhs));
    }

    inline
    media operator & (media lhs, media rhs) {
        return static_cast<media>(bitflag_t(lhs) & bitflag_t(rhs));
    }

} // sdk
