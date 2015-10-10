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

QString findAdultTitle(const QString& subject)
{
    QString ret;

    // Butterfly.2008.XXX.HDTVRiP.x264-REDX"
    if (tryCapture(subject, "(^\\S*)\\.(\\d{4})\\.XXX\\.", 1, ret))
        return ret;

    if (tryCapture(subject, "(^\\S*)\\.((FRENCH)|(GERMAN))\\.XXX\\.", 1, ret))
        return ret;

    // Boning.The.Mom.Next.Door.XXX.COMPLETE.NTSC.DVDR-DFA
    if (tryCapture(subject, "(^\\S*)\\.XXX\\.", 1, ret))
        return ret;

    return ret;
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
    if (pattern.indexIn(subject) != -1)
    {
        if (season)
            *season = pattern.cap(2);
        if (episode)
            *episode = pattern.cap(3);
        return pattern.cap(1);
    }


    return "";

}

QString toString(MediaType m) 
{
    using Media = MediaType;
    switch (m)
    {
        case Media::GamesNDS:        return "Nintento DS";
        case Media::GamesWii:        return "Nintendo Wii";
        case Media::GamesXbox:       return "Xbox";
        case Media::GamesXbox360:    return "XBox 360";
        case Media::GamesPSP:        return "Playstation Portable";
        case Media::GamesPS1:        return "Playstation 1";
        case Media::GamesPS2:        return "Playstation 2";
        case Media::GamesPS3:        return "Playstation 3";
        case Media::GamesPS4:        return "Playstation 4";
        case Media::GamesOther:      return "Games";

        case Media::MoviesInt:         return "Movies Int.";
        case Media::MoviesSD:          return "Movies SD";
        case Media::MoviesHD:          return "Movies HD";
        case Media::MoviesWMV:         return "Movies WMV";
        case Media::MoviesOther:       return "Movies";

        case Media::MusicMp3:          return "Music Mp3";
        case Media::MusicVideo:        return "Music Videos";
        case Media::MusicLossless:     return "Music Lossless";
        case Media::MusicOther:        return "Music";

        case Media::AppsPC:            return "Apps PC";
        case Media::AppsISO:           return "Apps ISO";
        case Media::AppsMac:           return "Apps Mac";
        case Media::AppsAndroid:       return "Apps Android";
        case Media::AppsIos:           return "Apps iOS";
        case Media::AppsOther:         return "Apps";

        case Media::TvInt:             return "TV Int.";
        case Media::TvSD:              return "TV SD";
        case Media::TvHD:              return "TV HD";
        case Media::TvSport:           return "TV Sports";
        case Media::TvOther:           return "TV";        

        case Media::AdultDVD:          return "Adult DVD";
        case Media::AdultHD:           return "Adult HD";
        case Media::AdultSD:           return "Adult SD";
        case Media::AdultImg:          return "Adult Img.";
        case Media::AdultOther:        return "Adult";

        case Media::Ebook:             return "EBook";
        case Media::Images:            return "Images";
        case Media::Other:             return "Other";
        case Media::Audiobook:         return "Audiobook";

        case Media::SENTINEL: break;
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
        case m::GamesNDS:
        case m::GamesWii:
        case m::GamesXbox:
        case m::GamesXbox360:
        case m::GamesPSP:
        case m::GamesPS1:
        case m::GamesPS2:
        case m::GamesPS3:
        case m::GamesPS4:
        case m::GamesOther:
            return QIcon("icons:ico_media_console.png");

        case m::MoviesInt:
        case m::MoviesSD:
        case m::MoviesHD:
        case m::MoviesWMV:
        case m::MoviesOther:
            return QIcon("icons:ico_media_movie.png");

        case m::MusicMp3:
        case m::MusicVideo:
        case m::MusicLossless:
        case m::MusicOther:
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

        case m::AdultDVD:
        case m::AdultHD:
        case m::AdultSD:
        case m::AdultImg:
        case m::AdultOther:
            return QIcon("icons:ico_media_xxx.png");

        case m::Images:
            return QIcon("icons:ico_media_image.png");

        case m::Audiobook:
        case m::Ebook:
        case m::Other:
            return QIcon("icons:ico_media_other.png");

        case m::SENTINEL:
            break;
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
        {MediaType::GamesNDS, "nintendo.ds"},
        {MediaType::GamesNDS, "nintendods"},
        {MediaType::GamesNDS, "nintendo3ds"},
        {MediaType::GamesWii, "games.wii."},
        {MediaType::GamesWii, "binaries.wii."},
        {MediaType::GamesWii, "nintendo.wii."},
        {MediaType::GamesPS2, "playstation2"},
        {MediaType::GamesPS3, "playstation3"},
        {MediaType::GamesPS1, "playstation"},        
        {MediaType::GamesPS3, "console.ps3"},
        {MediaType::GamesPSP, "console.psp"},
        {MediaType::GamesXbox360, "games.xbox360"},
        {MediaType::GamesXbox360, "binaries.xbox360"},
        {MediaType::GamesXbox, "games.xbox"},
        {MediaType::GamesXbox, "binaries.xbox"},
        {MediaType::GamesOther, ".console."},        
        {MediaType::GamesOther, ".consoles."},                

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
        {MediaType::MoviesOther, "binaries.movies"},
        {MediaType::MoviesOther, ".movie."},
        {MediaType::MoviesOther, ".movies."},

        {MediaType::MusicMp3, ".mp3"},
        {MediaType::MusicVideo, "music.videos"},
        {MediaType::MusicVideo, "video.music"},
        {MediaType::MusicVideo, "videos.music"},
        {MediaType::MusicVideo, "dvd.music"},
        {MediaType::MusicVideo, "music.dvd"},
        {MediaType::MusicLossless, "music.flac"},
        {MediaType::MusicLossless, "sounds.flac"},        
        {MediaType::MusicLossless, "music.lossless"},
        {MediaType::MusicLossless, "sounds.lossless"},
        {MediaType::MusicLossless, "binaries.lossless"},
        {MediaType::MusicOther, "binaries.music"},
        {MediaType::MusicOther, ".audio."},
        {MediaType::MusicOther, ".sound."},
        {MediaType::MusicOther, ".sounds."},        


        {MediaType::AppsPC, "win95-apps"},
        {MediaType::AppsPC, "cd.image.winapps"},
        {MediaType::AppsPC, "osx.apps"},
        {MediaType::AppsISO, "binaries.iso"},        
        {MediaType::AppsISO, "binaries.cd.images"},
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
        {MediaType::TvOther, "binaries.teevee"},
        {MediaType::TvOther, "tvshows"},
        {MediaType::TvOther, ".tvseries."},
        {MediaType::TvOther, ".tv."},

        {MediaType::AdultDVD, "erotica.dvd"},
        {MediaType::AdultDVD, "dvd.erotica"},
        {MediaType::AdultSD, "erotica.divx"},
        {MediaType::AdultImg, "erotica.pictures"},
        {MediaType::AdultImg, "pictures.erotic"},
        {MediaType::AdultImg, "pictures.erotica"},
        {MediaType::AdultImg, "pictures.fetish"},
        {MediaType::AdultImg, "pictures.sex"},
        {MediaType::AdultImg, "pictures.nude"},
        {MediaType::AdultImg, "pictures.female"},
        {MediaType::AdultOther, ".binaries.erotica"},
        {MediaType::AdultOther, ".multimedia.erotica"},                
        {MediaType::AdultOther, ".erotica."},

        {MediaType::Ebook, ".ebook."},
        {MediaType::Ebook, ".ebooks."},
        {MediaType::Audiobook, "audiobooks"},
        {MediaType::Images, "binaries.pictures"}
    };

    for (const auto& it : patterns)
    {
        //if (!newsgroup.contains(".binaries."))
        //    continue;

        if (newsgroup.contains(it.pattern))
            return it.type;
    }

    return MediaType::Other;
}

} // app
