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
    enum class media {
        //console,
        console_nds,    
        console_wii,    
        console_xbox,   
        console_xbox360,
        console_psp,    
        console_ps2,    
        console_ps3,   
        console_ps4,   

        //movies,
        movies_int,
        movies_sd, 
        movies_hd, 
        
        //audio,
        audio_mp3,
        audio_video,
        audio_audiobook,
        audio_lossless,

        //apps,
        apps_pc,    
        apps_iso,   
        apps_mac,   
        apps_android,
        apps_ios,    

        //tv,
        tv_int,
        tv_sd, 
        tv_hd, 
        tv_other,
        tv_sport, 

        //xxx,
        xxx_dvd, 
        xxx_hd,
        xxx_sd,

        //other
        ebook,

        // needs to be last value!
        sentinel
    };


    // media item. these items are retrieved from RSS feeds/newznab etc. searches.
    struct mediaitem {

        // media tag flags set on this item
        media type;

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
    class media_iterator : public std::iterator<std::forward_iterator_tag, media>
    {
    public:
        media_iterator(media beg) : cur_(beg)
        {}
        media_iterator() : cur_(media::sentinel)
        {}

        // postfix
        media_iterator operator ++(int)
        {
            media_iterator tmp(cur_);
            auto value = (unsigned)cur_;
            ++value;
            cur_ = (media)value;
            return tmp;
        }

        media_iterator& operator++()
        {
            auto value = (unsigned)cur_;
            ++value;
            cur_ = (media)value;
            return *this;
        }
        media operator*() const 
        {
            return cur_;
        }
        media& operator*()
        {
            return cur_;
        }
        static 
        media_iterator begin() 
        {
            return media_iterator(media::console_nds);
        }
        static
        media_iterator end() 
        {
            return media_iterator(media::sentinel);
        }

    private:
        friend bool operator==(const media_iterator&, const media_iterator&);
    private:
        media cur_;
    };

    inline
    bool operator==(const media_iterator& lhs, const media_iterator& rhs)
    {
        return lhs.cur_ == rhs.cur_;
    }
    inline 
    bool operator!=(const media_iterator& lhs, const media_iterator& rhs)
    {
        return !(lhs == rhs);
    }

    // stringify media tag name
    const char* str(media m);

} // app
