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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtGui/QIcon>
#  include <QString>
#  include <QDateTime>
#include "newsflash/warnpop.h"

#include <iterator>

#include "engine/bitflag.h"

namespace app
{
    enum class MediaSource {
        // the item is from a RSS stream
        RSS,
        // the item is from a search
        Search,

        // the item is from headers.
        Headers,

        // the item is from a nzb file
        File
    };

    // media types tags that apply to any particular media object.
    // coming from RSS feeds, Newznab etc.
    // int = international
    // sd  = standard definition
    // hd  = high definition

    // todo: media types should be ideally be defined as a type
    // and a bunch of "tags" so that a movie can be tagged for example
    // can be tagged with the appropriate tags such as
    // "hd, 1080, mkv, x264, french" for a High definition x264 encoded french movie.
    // unfortunately this is not the reality with the indexers.


    // version number for the MediaType
    // if the Enum is reorganized (new values added to the middle, etc.)
    // the version number needs to be bumped
    // and code that relies on a particular version needs to be updated
    // to account for the new version.
    enum {
        MediaTypeVersion = 2
    };

    // This is a parent category enum for the MediaType below.
    enum class MainMediaType {
        Adult,
        Apps,
        Music,
        Games,
        Images,
        Movies,
        Television,
        Other
    };

    enum class MediaType {
        // adult content
        AdultDVD,
        AdultHD,
        AdultSD,
        AdultImg,
        AdultOther,

        //apps,
        AppsAndroid,
        AppsMac,
        AppsIos,
        AppsISO,
        AppsPC,
        AppsOther,

        MusicLossless,
        MusicMp3,
        MusicVideo,
        MusicOther,

        //console (games)
        GamesNDS,
        GamesPSP,
        GamesPS1,
        GamesPS2,
        GamesPS3,
        GamesPS4,
        GamesWii,
        GamesXbox,
        GamesXbox360,
        GamesOther,

        Images,

        //movies,
        MoviesInt,
        MoviesSD,
        MoviesHD,
        MoviesWMV,
        MoviesOther,

        //tv,
        TvInt,
        TvSD,
        TvHD,
        TvSport,
        TvOther,

        //other
        Ebook,
        Audiobook,
        Other,

        // needs to be the last value.
        SENTINEL
    };

    using MediaTypeFlag = newsflash::bitflag<MediaType, std::uint64_t>;

    inline
    bool isMovie(MediaType type)
    {
        return type == MediaType::MoviesInt ||
            type == MediaType::MoviesSD ||
            type == MediaType::MoviesHD ||
            type == MediaType::MoviesWMV ||
            type == MediaType::MoviesOther;
    }

    inline
    bool isTelevision(MediaType type)
    {
        return type == MediaType::TvInt ||
            type == MediaType::TvSD ||
            type == MediaType::TvHD ||
            type == MediaType::TvOther ||
            type == MediaType::TvSport;
    }

    inline
    bool isMusic(MediaType type)
    {
        return type == MediaType::MusicMp3 ||
            type == MediaType::MusicVideo ||
            type == MediaType::MusicLossless ||
            type == MediaType::MusicOther;
    }

    inline
    bool isConsole(MediaType type)
    {
        return type == MediaType::GamesNDS ||
            type == MediaType::GamesWii ||
            type == MediaType::GamesXbox ||
            type == MediaType::GamesXbox360 ||
            type == MediaType::GamesPSP ||
            type == MediaType::GamesPS1 ||
            type == MediaType::GamesPS2 ||
            type == MediaType::GamesPS3 ||
            type == MediaType::GamesPS4 ||
            type == MediaType::GamesOther;
    }

    inline
    bool isApps(MediaType type)
    {
        return type == MediaType::AppsPC ||
            type == MediaType::AppsISO ||
            type == MediaType::AppsMac ||
            type == MediaType::AppsAndroid ||
            type == MediaType::AppsIos ||
            type == MediaType::AppsOther;
    }

    inline
    bool isAdult(MediaType type)
    {
        return type == MediaType::AdultDVD ||
            type == MediaType::AdultHD ||
            type == MediaType::AdultSD ||
            type == MediaType::AdultImg ||
            type == MediaType::AdultOther;
    }

    inline
    bool isImage(MediaType type)
    {
        return type == MediaType::Images;
    }

    inline
    bool isOther(MediaType type)
    {
        return type == MediaType::Ebook ||
            type == MediaType::Audiobook ||
            type == MediaType::Other;
    }


    // media item. these items are retrieved from RSS feeds/newznab etc. searches.
    struct MediaItem {

        // media tag flags set on this item
        MediaType type = MediaType::Other;

        // human readable title
        QString title;

        // globally unique item id
        QString guid;

        // link to the NZB file content
        QString nzblink;

        // publishing date and time
        QDateTime pubdate;

        // size in bytes if known, otherwise 0
        quint64 size = 0;

        // true if password is required for the item
        bool password = false;
    };

    // media iterator to iterate over media tags.
    class MediaIterator : public std::iterator<std::forward_iterator_tag, MediaType>
    {
    public:
        MediaIterator(unsigned value) : value_(value)
        {}
        MediaIterator() : value_((unsigned)MediaType::SENTINEL)
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
            return MediaIterator((unsigned)MediaType::AdultDVD);
        }
        static
        MediaIterator end()
        {
            return MediaIterator((unsigned)MediaType::SENTINEL);
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


    QString findAdultTitle(const QString& subject);
    QString findMovieTitle(const QString& subject, QString* outReleaseYear = nullptr);
    QString findTVSeriesTitle(const QString& subject, QString* season = nullptr, QString* episode =  nullptr);

    QString toString(MediaType media);
    QString toString(MediaSource source);

    QIcon toIcon(MediaType media);
    QIcon toIcon(MediaSource source);

    MediaType findMediaType(const QString& newsgroup);

    MainMediaType toMainType(MediaType type);

} // app
