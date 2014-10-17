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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QString>
#include <newsflash/warnpop.h>

class QIcon;
class QString;

namespace app
{
    // filetypes categorizes files into broad categories
    // of possible types.
    enum class filetype {
        none, 

        // audio and music files such as .mp3, .ogg, .flac
        audio, 

        // videos and movies such as .mkv, .wmv, .avi
        video,

        // images and pictures such as .jpg, .png
        image, 

        // textual contents such as .txt and .nfo
        text, 

        // data archive files such as .zip and .rar
        archive, 

        // parity i.e. par2 files. 
        parity, 

        // document files such as .pdf, .doc, .xml
        document, 

        // unknown filetype
        other
    };

    // get the default hardcoded filepattern string for the given filetype
    const char* filepattern(filetype type);

    const char* str(filetype type);

    // try to identify file's type.
    filetype find_filetype(const QString& filename);

    QIcon iconify(filetype type);

} // app