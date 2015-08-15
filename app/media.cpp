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

bool tryCapture(const QString& subject, const QString& regex, int index, QString& out)
{
    const QRegExp pattern(regex);
    if (pattern.indexIn(subject) == -1)
        return false;

    out = pattern.cap(index);
    return true;
}

QString findMovieTitle(const QString& subject)
{
    QString ret;

    // Two.Girls.And.a.Guy.1997.1080p.BluRay.x264-BARC0DE    
    if (tryCapture(subject, "(^\\S*)\\.(\\d{4})\\.", 1, ret))
        return ret;

    // Dark.Salvation.DVDRip.Multi4.READ
    if (tryCapture(subject, "(^\\S*)\\.(DVDRIP)", 1, ret))
        return ret;
    if (tryCapture(subject, "(^\\S*)\\.(DVDRip)", 1, ret))
        return ret;

    return ret;    
}

QString findTVSeriesTitle(const QString& subject, QString* season, QString* episode)
{
    // Betas.S01E05.1080p.WEBRip.H264-BATV
    const QRegExp pattern("^(\\S*)\\.S(\\d{2})E(\\d{2})\\.");
    if (pattern.indexIn(subject) == -1)
        return "";

    if (season)
        *season = pattern.cap(2);
    if (episode)
        *episode = pattern.cap(3);

    return pattern.cap(1);
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
        case Media::ConsolePS1:        return "Playstation 1";
        case Media::ConsolePS2:        return "Playstation 2";
        case Media::ConsolePS3:        return "Playstation 3";
        case Media::ConsolePS4:        return "Playstation 4";
        case Media::ConsoleOther:      return "Console";

        case Media::MoviesInt:         return "Movies Int.";
        case Media::MoviesSD:          return "Movies SD";
        case Media::MoviesHD:          return "Movies HD";
        case Media::MoviesWMV:         return "Movies WMV";
        case Media::MoviesOther:       return "Movies";

        case Media::AudioMp3:          return "Mp3";
        case Media::AudioVideo:        return "Music Videos";
        case Media::AudioAudiobook:    return "Audiobook";
        case Media::AudioLossless:     return "Lossless Audio";
        case Media::AudioOther:        return "Audio";

        case Media::AppsPC:            return "PC";
        case Media::AppsISO:           return "ISO";
        case Media::AppsMac:           return "Mac";
        case Media::AppsAndroid:       return "Android";
        case Media::AppsIos:           return "iOS";
        case Media::AppsOther:         return "Apps";

        case Media::TvInt:             return "TV Int.";
        case Media::TvSD:              return "TV SD";
        case Media::TvHD:              return "TV HD";
        case Media::TvOther:           return "TV Other";
        case Media::TvSport:           return "TV Sports";

        case Media::XxxDVD:            return "XXX DVD";
        case Media::XxxHD:             return "XXX HD";
        case Media::XxxSD:             return "XXX SD";
        case Media::XxxImg:            return "XXX Img.";
        case Media::XxxOther:          return "XXX";

        case Media::Ebook:             return "EBook";
        case Media::Images:            return "Images";
        case Media::Other:             return "Other";
    }
    Q_ASSERT(false);
    return {};
}

QString toString(MediaSource source)
{
    switch (source)
    {
        case MediaSource::RSS:     return "RSS";
        case MediaSource::Search:  return "Search";
        case MediaSource::Headers: return "News";
        case MediaSource::File:    return "File";
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
        case m::ConsolePS1:
        case m::ConsolePS2:
        case m::ConsolePS3:
        case m::ConsolePS4:
        case m::ConsoleOther:
            return QIcon("icons:ico_media_console.png");

        case m::MoviesInt:
        case m::MoviesSD:
        case m::MoviesHD:
        case m::MoviesWMV:
        case m::MoviesOther:
            return QIcon("icons:ico_media_movie.png");

        case m::AudioMp3:
        case m::AudioVideo:
        case m::AudioAudiobook:
        case m::AudioLossless:
        case m::AudioOther:
            return QIcon("icons:ico_media_audio.png");

        case m::AppsPC:
        case m::AppsISO:
        case m::AppsMac:
        case m::AppsAndroid:
        case m::AppsIos:
        case m::AppsOther:
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
        case m::XxxImg:
        case m::XxxOther:
            return QIcon("icons:ico_media_xxx.png");

        case m::Images:
            return QIcon("icons:ico_media_image.png");

        case m::Ebook:
        case m::Other:

            return QIcon("icons:ico_media_other.png");
    }
    Q_ASSERT(false);
    return {};
}

QIcon toIcon(MediaSource source)
{
    switch (source)
    {
        case MediaSource::RSS: return QIcon("icons:ico_rss.png");
        case MediaSource::Search: return QIcon("icons:ico_search.png");
        case MediaSource::Headers: return QIcon("icons:ico_news.png");
        case MediaSource::File: return QIcon("icons:ico_nzb.png");
    }
    Q_ASSERT(false);
    return {};
}

MediaType findMediaType(const QString& newsgroup)
{
    struct map {
        MediaType type;
        QString pattern;
    } patterns[] = {
        {MediaType::ConsoleNDS, "nintendo.ds"},
        {MediaType::ConsoleNDS, "nintendods"},
        {MediaType::ConsoleNDS, "nintendo3ds"},
        {MediaType::ConsoleWii, "games.wii."},
        {MediaType::ConsoleWii, "binaries.wii."},
        {MediaType::ConsoleWii, "nintendo.wii."},
        {MediaType::ConsolePS2, "playstation2"},
        {MediaType::ConsolePS3, "playstation3"},
        {MediaType::ConsolePS1, "playstation"},        
        {MediaType::ConsolePS3, "console.ps3"},
        {MediaType::ConsolePSP, "console.psp"},
        {MediaType::ConsoleXbox360, "games.xbox360"},
        {MediaType::ConsoleXbox360, "binaries.xbox360"},
        {MediaType::ConsoleXbox, "games.xbox"},
        {MediaType::ConsoleXbox, "binaries.xbox"},
        {MediaType::ConsoleOther, ".console."},        
        {MediaType::ConsoleOther, ".consoles."},                

        {MediaType::MoviesInt, "movies.french"},
        {MediaType::MoviesInt, "movies.divx.french"},        
        {MediaType::MoviesInt, "movies.german"},
        {MediaType::MoviesInt, "movies.divx.german"},        
        {MediaType::MoviesInt, "movies.spanish"},
        {MediaType::MoviesInt, "movies.divx.spanish"},
        {MediaType::MoviesInt, "movies.italian"},
        {MediaType::MoviesInt, "movies.divx.italian"},        
        {MediaType::MoviesInt, "movies.divx.turkish"},                
        {MediaType::MoviesInt, "movies.divx.russian"},                        
        {MediaType::MoviesSD, "movies.divx"},
        {MediaType::MoviesSD, "movies.xvid"},        
        {MediaType::MoviesSD, "movies.dvd"},
        {MediaType::MoviesHD, "movies.x264"},
        {MediaType::MoviesHD, "movies.mkv"},
        {MediaType::MoviesHD, "binaries.bluray"},
        {MediaType::MoviesHD, "binaries.blu-ray"},
        {MediaType::MoviesWMV, "movies.wmv"},
        {MediaType::MoviesWMV, "binaries.wmv"},
        {MediaType::MoviesWMV, "binaries.wmvhd"},
        {MediaType::MoviesWMV, "binaries.wmv-hd"},
        {MediaType::MoviesOther, ".movie."},
        {MediaType::MoviesOther, ".movies."},

        {MediaType::AudioMp3, ".mp3"},
        {MediaType::AudioVideo, "music.videos"},
        {MediaType::AudioVideo, "video.music"},
        {MediaType::AudioVideo, "videos.music"},
        {MediaType::AudioVideo, "dvd.music"},
        {MediaType::AudioVideo, "music.dvd"},
        {MediaType::AudioLossless, "music.flac"},
        {MediaType::AudioLossless, "sounds.flac"},        
        {MediaType::AudioLossless, "music.lossless"},
        {MediaType::AudioLossless, "sounds.lossless"},
        {MediaType::AudioLossless, "binaries.lossless"},
        {MediaType::AudioAudiobook, "audiobooks"},
        {MediaType::AudioOther, ".audio."},
        {MediaType::AudioOther, ".sound."},
        {MediaType::AudioOther, ".sounds."},        


        {MediaType::AppsPC, "win95-apps"},
        {MediaType::AppsPC, "osx.apps"},
        {MediaType::AppsISO, "binaries.iso"},        
        {MediaType::AppsISO, "linux.iso"},        
        {MediaType::AppsISO, "image.iso"},
        {MediaType::AppsISO, "german.iso"},        
        {MediaType::AppsAndroid, "alt.binaries.android"},
        {MediaType::AppsMac, "alt.binaries.mac"},
        {MediaType::AppsMac, ".mac.applications"},
        {MediaType::AppsMac, ".mac.apps"},        
        {MediaType::AppsMac, ".mac.osx"},
        {MediaType::AppsMac, ".max.games"},
        {MediaType::Other, "binaries.applications"},
        {MediaType::AppsOther, "binaries.apps"},

        {MediaType::TvInt, ".tv.deutsch"},
        {MediaType::TvInt, ".tv.french"},
        {MediaType::TvInt, ".tv.swedish"},
        {MediaType::TvInt, ".hdtv.swedish"},        
        {MediaType::TvInt, ".hdtv.german"},                
        {MediaType::TvInt, ".hdtv.french"},                
        {MediaType::TvInt, ".hdtv.spanish"},                        
        {MediaType::TvSD, ".tv.divx"},
        {MediaType::TvHD, "binaries.hdtv"},
        {MediaType::TvHD, ".hdtv."},
        {MediaType::TvSport, "multimedia.sports"},        
        {MediaType::TvOther, "binaries.appletv"},
        {MediaType::TvOther, "tvshows"},
        {MediaType::TvOther, ".tvseries."},
        {MediaType::TvOther, ".tv."},


        {MediaType::XxxDVD, "erotica.dvd"},
        {MediaType::XxxDVD, "dvd.erotica"},
        {MediaType::XxxSD, "erotica.divx"},
        {MediaType::XxxImg, "erotica.pictures"},
        {MediaType::XxxImg, "pictures.erotica"},
        {MediaType::XxxOther, ".binaries.erotica"},
        {MediaType::XxxOther, ".multimedia.erotica"},                
        {MediaType::XxxOther, ".erotica."},


        {MediaType::Ebook, ".ebook."},
        {MediaType::Ebook, ".ebooks."},

        {MediaType::Images, "binaries.pictures"}

    };

    for (const auto& it : patterns)
    {
        if (newsgroup.contains(it.pattern))
            return it.type;
    }

    return MediaType::Other;
}

} // app
