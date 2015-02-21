// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
// 
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QString>
#include <newsflash/warnpop.h>
#include <iterator>

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
        other,
    };

    class filetype_iterator : public std::iterator<std::forward_iterator_tag, filetype>
    {
    public:
        filetype_iterator(filetype beg) : value_(unsigned(beg))
        {}
        filetype_iterator() : value_(unsigned(filetype::other) + 1)
        {}

        // postfix
        filetype_iterator operator++(int)
        {
            filetype_iterator tmp(value_);
            ++value_;
            return tmp;
        }

        // prefix
        filetype_iterator& operator++()
        {
            ++value_;
            return *this;
        }

        filetype operator*() const 
        { return (filetype)value_; }

        static
        filetype_iterator begin()
        { return filetype_iterator(filetype::audio); }

        static
        filetype_iterator end() 
        { return filetype_iterator(); }

    private:
        filetype_iterator(unsigned value) : value_(value)
        {}

    private:
        friend bool operator==(const filetype_iterator&, const filetype_iterator&);
    private:
        unsigned value_;
    };

    inline
    bool operator==(const filetype_iterator& lhs, const filetype_iterator& rhs)
    {
        return lhs.value_ == rhs.value_;
    }

    inline
    bool operator!=(const filetype_iterator& lhs, const filetype_iterator& rhs)
    {
        return !(lhs == rhs);
    }

    // get the default hardcoded filepattern string for the given filetype
    const char* filepattern(filetype type);

    const char* str(filetype type);

    // try to identify file's type.
    filetype findFileType(const QString& filename);

    QIcon findFileIcon(filetype type);



} // app