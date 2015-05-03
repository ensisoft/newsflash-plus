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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QRegExp>
#include <newsflash/warnpop.h>
#include "media.h"

namespace app
{

QString findMovieTitle(const QString& subject)
{
    // match something like
    // Two.Girls.And.a.Guy.1997.1080p.BluRay.x264-BARC0DE
    const QRegExp pattern("(^\\S*)\\.(\\d{4})\\.");

    if (pattern.indexIn(subject) == -1)
        return "";

    const auto title = pattern.cap(1);
    return title;
}

QString toString(MediaType m) 
{
    using Media = MediaType;
    switch (m)
    {
        case Media::ConsoleNDS:        return "Nintento DS";
        case Media::ConsoleWii:        return "Nintendo Wii";
        case Media::ConsoleXbox:       return "Xbox";
        case Media::ConsoleXbox360:    return "XBox 360";
        case Media::ConsolePSP:        return "Playstation Portable";
        case Media::ConsolePS2:        return "Playstation 2";
        case Media::ConsolePS3:        return "Playstation 3";
        case Media::ConsolePS4:        return "Playstation 4";
        case Media::MoviesInt:         return "Movies Int.";
        case Media::MoviesSD:          return "Movies SD";
        case Media::MoviesHD:          return "Movies HD";
        case Media::MoviesWMV:         return "Movies WMV";
        case Media::AudioMp3:          return "Mp3";
        case Media::AudioVideo:        return "Music Videos";
        case Media::AudioAudiobook:    return "Audiobook";
        case Media::AudioLossless:     return "Lossless Audio";
        case Media::AppsPC:            return "PC";
        case Media::AppsISO:           return "ISO";
        case Media::AppsMac:           return "Mac";
        case Media::AppsAndroid:       return "Android";
        case Media::AppsIos:           return "iOS";
        case Media::TvInt:             return "TV Int.";
        case Media::TvSD:              return "TV SD";
        case Media::TvHD:              return "TV HD";
        case Media::TvOther:           return "TV Other";
        case Media::TvSport:           return "TV Sports";
        case Media::XxxDVD:            return "XXX DVD";
        case Media::XxxHD:             return "XXX HD";
        case Media::XxxSD:             return "XXX SD";
        case Media::XxxOther:          return "XXX Other";
        case Media::Ebook:             return "EBook";
        case Media::Other:             return "Other";
    }
    Q_ASSERT(false);
    return {};
}

QIcon toIcon(MediaType type)
{
    using m = MediaType;
    switch (type)
    {
        case m::ConsoleNDS:
        case m::ConsoleWii:
        case m::ConsoleXbox:
        case m::ConsoleXbox360:
        case m::ConsolePSP:
        case m::ConsolePS2:
        case m::ConsolePS3:
        case m::ConsolePS4:
            return QIcon("icons:ico_media_console.png");

        case m::MoviesInt:
        case m::MoviesSD:
        case m::MoviesHD:
        case m::MoviesWMV:
            return QIcon("icons:ico_media_movie.png");

        case m::AudioMp3:
        case m::AudioVideo:
        case m::AudioAudiobook:
        case m::AudioLossless:
            return QIcon("icons:ico_media_audio.png");

        case m::AppsPC:
        case m::AppsISO:
        case m::AppsMac:
        case m::AppsAndroid:
        case m::AppsIos:
            return QIcon("icons:ico_media_apps.png");

        case m::TvInt:
        case m::TvSD:
        case m::TvHD:
        case m::TvOther:
        case m::TvSport:
            return QIcon("icons:ico_media_tv.png");

        case m::XxxDVD:
        case m::XxxHD:
        case m::XxxSD:
        case m::XxxOther:
            return QIcon("icons:ico_media_xxx.png");

        case m::Ebook:
        case m::Other:
            return QIcon("icons:ico_media_other.png");
    }
    Q_ASSERT(false);
    return {};
}


} // app
