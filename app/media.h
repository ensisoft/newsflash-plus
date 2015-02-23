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
// The above copyright notice and this perioimission notice shall be included in
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
#  include <QDateTime>
#include <newsflash/warnpop.h>
#include <newsflash/engine/bitflag.h>
#include <iterator>

namespace app
{
    // media types tags that apply to any particular media object.
    // coming from RSS feeds, Newznab etc.
    // int = international
    // sd  = standard definition
    // hd  = high definition
    enum class Media {
        //console,
        ConsoleNDS,    
        ConsoleWii,    
        ConsoleXbox,   
        ConsoleXbox360,
        ConsolePSP,    
        ConsolePS2,    
        ConsolePS3,   
        ConsolePS4,   

        //movies,
        MoviesInt,
        MoviesSD, 
        MoviesHD,
        MoviesWMV,
        
        //audio,
        AudioMp3,
        AudioVideo,
        AudioAudiobook,
        AudioLossless,

        //apps,
        AppsPC,    
        AppsISO,   
        AppsMac,   
        AppsAndroid,
        AppsIos,    

        //tv,
        TvInt,
        TvSD, 
        TvHD, 
        TvOther,
        TvSport, 

        //xxx,
        XxxDVD,
        XxxHD,
        XxxSD,

        //other
        Ebook
    };


    // media item. these items are retrieved from RSS feeds/newznab etc. searches.
    struct MediaItem {

        // media tag flags set on this item
        Media type;

        // human readable title
        QString title;

        // globally unique item id
        QString gid;

        // link to the NZB file content
        QString nzblink;

        // publishing date and time
        QDateTime pubdate;

        // size in bytes if known, otherwise 0
        quint64 size;

        // true if password is required for the item
        bool password;
    };

    // media iterator to iterate over media tags.
    class MediaIterator : public std::iterator<std::forward_iterator_tag, Media>
    {
    public:
        MediaIterator(unsigned value) : value_(value)
        {}
        MediaIterator() : value_((unsigned)Media::Ebook + 1)
        {}

        // postfix
        MediaIterator operator ++(int)
        {
            MediaIterator tmp(value_);
            ++value_;
            return tmp;
        }

        MediaIterator& operator++()
        {
            ++value_;
            return *this;
        }
        Media operator*() const 
        {
            return (Media)value_;
        }
        Media& operator*()
        {
            return (Media&)value_;
        }
        static 
        MediaIterator begin() 
        {
            return MediaIterator((unsigned)Media::ConsoleNDS);
        }
        static
        MediaIterator end() 
        {
            return MediaIterator((unsigned)Media::Ebook + 1);
        }

    private:
        friend bool operator==(const MediaIterator&, const MediaIterator&);
    private:
        unsigned value_;
    };

    inline
    bool operator==(const MediaIterator& lhs, const MediaIterator& rhs)
    {
        return lhs.value_ == rhs.value_;
    }
    inline 
    bool operator!=(const MediaIterator& lhs, const MediaIterator& rhs)
    {
        return !(lhs == rhs);
    }


    QString findMovieTitle(const QString& subject);

} // app
