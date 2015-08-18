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
    enum {
        FileTypeVersion = 1
    };

    // filetypes categorizes files into broad categories
    // of possible types.
    enum class FileType {
        None, 

        // audio and music files such as .mp3, .ogg, .flac
        Audio, 

        // videos and movies such as .mkv, .wmv, .avi
        Video,

        // images and pictures such as .jpg, .png
        Image, 

        // textual contents such as .txt and .nfo
        Text, 

        // data archive files such as .zip and .rar
        Archive, 

        // parity i.e. par2 files. 
        Parity, 

        // document files such as .pdf, .doc, .xml
        Document, 

        // unknown filetype
        Other,
    };

    class FileTypeIterator : public std::iterator<std::forward_iterator_tag, FileType>
    {
    public:
        FileTypeIterator(FileType beg) : value_(unsigned(beg))
        {}
        FileTypeIterator() : value_(unsigned(FileType::Other) + 1)
        {}

        // postfix
        FileTypeIterator operator++(int)
        {
            FileTypeIterator tmp(value_);
            ++value_;
            return tmp;
        }

        // prefix
        FileTypeIterator& operator++()
        {
            ++value_;
            return *this;
        }

        FileType operator*() const 
        { return (FileType)value_; }

        static
        FileTypeIterator begin()
        { return FileTypeIterator(FileType::Audio); }

        static
        FileTypeIterator end() 
        { return FileTypeIterator(); }

    private:
        FileTypeIterator(unsigned value) : value_(value)
        {}

    private:
        friend bool operator==(const FileTypeIterator&, const FileTypeIterator&);
    private:
        unsigned value_;
    };

    inline
    bool operator==(const FileTypeIterator& lhs, const FileTypeIterator& rhs)
    {
        return lhs.value_ == rhs.value_;
    }

    inline
    bool operator!=(const FileTypeIterator& lhs, const FileTypeIterator& rhs)
    {
        return !(lhs == rhs);
    }

    // get the default hardcoded filepattern string for the given filetype
    QString filePattern(FileType type);

    QString toString(FileType type);

    // try to identify file's type.
    FileType findFileType(const QString& filename);

    // get filetype based on the extension.
    // if not known returns None.
    FileType fileType(const QString& ext);

    QIcon findFileIcon(FileType type);



} // app