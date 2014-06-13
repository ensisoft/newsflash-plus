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

#include <cassert>
#include "category.h"

namespace sdk
{
const char* str(category cat) 
{
    switch (cat)
    {
        case category::none: return "none";
        case category::console_nds: return "Nintento DS";
        case category::console_wii: return "Nintendo Wii";
        case category::console_xbox: return "Xbox";
        case category::console_xbox360: return "XBox 360";
        case category::console_psp: return "Playstation Portable";
        case category::console_ps2: return "Playstation 2";
        case category::console_ps3: return "Playstation 3";
        case category::console_ps4: return "Playstation 4";
        case category::movies_int: return "Movies Int.";
        case category::movies_sd: return "Movies SD";
        case category::movies_hd: return "Movies HD";
        case category::audio_mp3: return "Mp3";
        case category::audio_video: return "Music Videos";
        case category::audio_audiobook: return "Audiobook";
        case category::audio_lossless: return "Lossless Audio";
        case category::apps_pc: return "PC";
        case category::apps_iso: return "ISO";
        case category::apps_mac: return "Mac";
        case category::apps_android: return "Android";
        case category::apps_ios: return "iOS";
        case category::tv_int: return "TV Int.";
        case category::tv_sd: return "TV SD";
        case category::tv_hd: return "TV HD";
        case category::tv_other: return "TV Other";
        case category::tv_sport: return "TV Sports";
        case category::xxx_dvd: return "XXX DVD";
        case category::xxx_hd: return "XXX HD";
        case category::xxx_sd: return "XXX SD";
        case category::ebook: return "EBook";
        assert(!"unknown category");
    }
    return "???";
}



} // sdk