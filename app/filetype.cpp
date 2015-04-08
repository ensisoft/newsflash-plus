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
#  include <QtGui/QIcon>
#  include <QRegExp>
#  include <QString>
#include <newsflash/warnpop.h>

#include "filetype.h"

namespace app
{

QString filepattern(FileType type)
{
    switch (type)
    {
        case FileType::None:
            Q_ASSERT(!"none is not a valid filetype");
            break;
        case FileType::Audio:
            return ".mp3 | .mp2 | .wav | .ogg | .xm | .flac | .m3u | .mpa";
        case FileType::Video:
            return ".avi | .mkv | .ogm | .wmv | .wma | .mpe?g | .rm | .mov | .flv | .asf | .mp4 | .3gp | .3g2 | .m4v";
        case FileType::Image:
            return ".jpe?g | .bmp | .png | .gif";
        case FileType::Text:
            return ".txt | .nfo | .sfv | .log";
        case FileType::Archive:
            return ".zip | .rar | .7z | .r\\d{1,3} | .\\d{2}";
        case FileType::Parity:
            return ".par | .par2";
        case FileType::Document:
            return ".doc | .chm | .pdf";
        case FileType::Other:
            return ".*";
    }
    return {};
}

QString toString(FileType type)
{
    switch (type)
    {
        case FileType::None:     Q_ASSERT(0);
        case FileType::Audio:    return "Audio";
        case FileType::Video:    return "Video";
        case FileType::Image:    return "Image";
        case FileType::Text:     return "Text";
        case FileType::Archive:  return "Archive";
        case FileType::Parity:   return "Parity";
        case FileType::Document: return "Document";
        case FileType::Other:    return "Other";
    }
    Q_ASSERT(false);
    return {};
}


FileType findFileType(const QString& filename)
{
    struct map {
        FileType type;
        QString pattern;
    } patterns[] = {
        {FileType::Audio, filepattern(FileType::Audio)},
        {FileType::Video, filepattern(FileType::Video)},
        {FileType::Image, filepattern(FileType::Image)},
        {FileType::Text,  filepattern(FileType::Text)},
        {FileType::Archive, filepattern(FileType::Archive)},
        {FileType::Parity, filepattern(FileType::Parity)},
        {FileType::Document, filepattern(FileType::Document)}
    };

    for (auto it = std::begin(patterns); it != std::end(patterns); ++it)
    {
        const auto& tokens = it->pattern.split("|");
        for (int i=0; i<tokens.size(); ++i)
        {
            // match the end of the filestring only
            QRegExp regex(QString("\\%1$").arg(tokens[i].simplified()));
            regex.setCaseSensitivity(Qt::CaseInsensitive);
            if (regex.indexIn(filename) != -1)
                return it->type;
        }
    }
    return FileType::Other;
}

QIcon findFileIcon(FileType type)
{
    switch (type)
    {
        case FileType::None:
            Q_ASSERT(!"none is not a valid filetype");
        case FileType::Audio:
            return QIcon("icons:ico_filetype_audio.png");
        case FileType::Video:
            return QIcon("icons:ico_filetype_video.png");
        case FileType::Image:
            return QIcon("icons:ico_filetype_image.png");
        case FileType::Text:
            return QIcon("icons:ico_filetype_text.png");
        case FileType::Archive:
            return QIcon("icons:ico_filetype_archive.png");
        case FileType::Parity:
            return QIcon("icons:ico_filetype_parity.png");
        case FileType::Document:
            return QIcon("icons:ico_filetype_document.png");
        case FileType::Other:
            return QIcon("icons:ico_filetype_other.png");
    }

    Q_ASSERT(!"wat");

    return {};
}

} // app
