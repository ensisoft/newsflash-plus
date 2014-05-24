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

#pragma once

namespace sdk
{
    // media category/type. each item is traditionally
    // tagged into a single category. 
    // int = international
    // sd  = standard definition
    // hd  = high definition
    enum class mediatype 
    {
        none,

        console,
        console_nds,
        console_wii,
        console_xbox,
        console_xbox360,
        console_psp,
        console_ps2,
        console_ps3,
        console_ps4,

        movies,
        movies_int,
        movies_sd,
        movies_hd,

        audio,
        audio_mp3,
        audio_video,
        audio_audiobook,
        audio_lossless,

        apps,
        apps_pc,
        apps_iso,
        apps_mac,
        apps_android,
        apps_ios,

        tv,
        tv_int,
        tv_sd,
        tv_hd,
        tv_other,
        tv_sport,

        xxx,
        xxx_dvd,
        xxx_hd,
        xxx_sd,

        other,
        other_ebook
    };

} // sdk
