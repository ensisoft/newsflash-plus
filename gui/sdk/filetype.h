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

#include "types.h"

namespace sdk
{
    // Different filetypes and file categories.
    // Filetypes can be combined together with include_filetype
    enum class filetype 
    {
        none = 0,

        // file is audio, e.g. .mp3, .ogg, .flac         
        audio = 1 << 1,         

        // file is video, e.g. .wmv, .mkv, .avi
        video = 1 << 2,         

        // file is image, e.g. .jpg, .jpeg, .png        
        image = 1 << 3,         

        // file is text .txt        
        text = 1 << 4,          

        // file is log, .log        
        log = 1 << 5,           

        // file is archive, .zip, .rar, .z7
        archive = 1 << 6,       

        // file is parity par2        
        parity = 1 << 7,        

        // everything else        
        other = 1 << 8,     

        // file is a document (.doc, .pdf, .chm)            
        document = 1 << 9      
    };


    // include given filetype in the category bit vector
    inline 
    filetype operator | (filetype x, filetype y)
    {
        return static_cast<filetype>(bitflag_t(x) | bitflag_t(y));
    }

    inline
    filetype operator & (filetype x, filetype y)
    {
        return static_cast<filetype>(bitflag_t(x) & bitflag_t(y));
    }


} // sdk