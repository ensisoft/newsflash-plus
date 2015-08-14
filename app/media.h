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
#  include <QtGui/QIcon>
#  include <QString>
#  include <QDateTime>
#include <newsflash/warnpop.h>
#include <newsflash/engine/bitflag.h>
#include <iterator>

namespace app
{
    enum class MediaSource {
        RSS, Search, Headers, File
    };

    // media types tags that apply to any particular media object.
    // coming from RSS feeds, Newznab etc.
    // int = international
    // sd  = standard definition
    // hd  = high definition
    enum class MediaType {
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
        XxxOther,


        //other
        Ebook,

        Other
    };

    inline
    bool isMovie(MediaType type) 
    {
        return type == MediaType::MoviesInt ||
            type == MediaType::MoviesSD ||
            type == MediaType::MoviesHD ||
            type == MediaType::MoviesWMV;
    }

    inline 
    bool isTVSeries(MediaType type)
    {
        return type == MediaType::TvInt ||
            type == MediaType::TvSD ||
            type == MediaType::TvHD ||
            type == MediaType::TvOther ||
            type == MediaType::TvSport;
    }
    


    // media item. these items are retrieved from RSS feeds/newznab etc. searches.
    struct MediaItem {

        // media tag flags set on this item
        MediaType type;

        // human readable title
        QString title;

        // globally unique item id
        QString guid;

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
    class MediaIterator : public std::iterator<std::forward_iterator_tag, MediaType>
    {
    public:
        MediaIterator(unsigned value) : value_(value)
        {}
        MediaIterator() : value_((unsigned)MediaType::Ebook + 1)
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
        MediaType operator*() const 
        {
            return (MediaType)value_;
        }

        static 
        MediaIterator begin() 
        {
            return MediaIterator((unsigned)MediaType::ConsoleNDS);
        }
        static
        MediaIterator end() 
        {
            return MediaIterator((unsigned)MediaType::Ebook + 1);
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
    QString findTVSeriesTitle(const QString& subject, QString* season = nullptr, QString* episode =  nullptr);

    QString toString(MediaType media);
    QString toString(MediaSource source);

    QIcon toIcon(MediaType media);
    QIcon toIcon(MediaSource source);

} // app
