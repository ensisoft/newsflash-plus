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
#include "media.h"

namespace app
{
const char* str(media m) 
{
    switch (m)
    {
        case media::none: return "none";
        case media::console_nds: return "Nintento DS";
        case media::console_wii: return "Nintendo Wii";
        case media::console_xbox: return "Xbox";
        case media::console_xbox360: return "XBox 360";
        case media::console_psp: return "Playstation Portable";
        case media::console_ps2: return "Playstation 2";
        case media::console_ps3: return "Playstation 3";
        case media::console_ps4: return "Playstation 4";
        case media::console_playstation: return "Playstation";
        case media::console_nintendo: return "Nintendo";
        case media::console_microsoft: return "Microsoft Xbox/360";        
        case media::console: return "Console";
        case media::movies_int: return "Movies Int.";
        case media::movies_sd: return "Movies SD";
        case media::movies_hd: return "Movies HD";
        case media::movies: return "Movies";
        case media::audio_mp3: return "Mp3";
        case media::audio_video: return "Music Videos";
        case media::audio_audiobook: return "Audiobook";
        case media::audio_lossless: return "Lossless Audio";
        case media::audio: return "Audio";
        case media::apps_pc: return "PC";
        case media::apps_iso: return "ISO";
        case media::apps_mac: return "Mac";
        case media::apps_android: return "Android";
        case media::apps_ios: return "iOS";
        case media::apps: return "Apps";
        case media::tv_int: return "TV Int.";
        case media::tv_sd: return "TV SD";
        case media::tv_hd: return "TV HD";
        case media::tv_other: return "TV Other";
        case media::tv_sport: return "TV Sports";
        case media::tv: return "TV";
        case media::xxx_dvd: return "XXX DVD";
        case media::xxx_hd: return "XXX HD";
        case media::xxx_sd: return "XXX SD";
        case media::xxx: return "XXX";
        case media::ebook: return "EBook";
        case media::all: return "All";
        assert(!"unknown media");
    }
    return "???";
}

} // app
