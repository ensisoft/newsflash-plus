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
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#  include <QRegExp>
#  include <QString>
#include <newsflash/warnpop.h>

#include "filetype.h"

namespace app
{

const char* filepattern(filetype type)
{
    switch (type)
    {
        case filetype::none:
            Q_ASSERT(!"none is not a valid filetype");
            break;
        case filetype::audio:
            return ".mp3 | .mp2 | .wav | .ogg | .xm | .flac | .m3u | .mpa";
        case filetype::video:
            return ".avi | .mkv | .ogm | .wmv | .wma | .wma | .mpe?g | .rm | .mov | .flv | .asf | .mp3 | .3gp | .3g2";
        case filetype::image:
            return ".jpe?g | .bmp | .png | .gif";
        case filetype::text:
            return ".txt | .nfo | .sfv | .log";
        case filetype::archive:
            return ".zip | .rar | .7z | .r\\d{1,3} | .\\d{2}";
        case filetype::parity:
            return ".par | .par2";
        case filetype::document:
            return ".doc | .chm | .pdf";
        case filetype::other:
            return ".*";
    }
    return "";
}

const char* str(filetype type)
{
    switch (type)
    {
        case filetype::none:
            return "none";
        case filetype::audio:
            return "Audio";
        case filetype::video:
            return "Video";
        case filetype::image:
            return "Image";
        case filetype::text:
            return "Text";
        case filetype::archive:
            return "Archive";
        case filetype::parity:
            return "Parity";
        case filetype::document:
            return "Document";
        case filetype::other:
            return "Other";
    }
    return "";
}

filetype findFileType(const QString& filename)
{
    struct map {
        filetype type;
        QString pattern;
    } patterns[] = {
        {filetype::audio, filepattern(filetype::audio)},
        {filetype::video, filepattern(filetype::video)},
        {filetype::image, filepattern(filetype::image)},
        {filetype::text,  filepattern(filetype::text)},
        {filetype::archive, filepattern(filetype::archive)},
        {filetype::parity, filepattern(filetype::parity)},
        {filetype::document, filepattern(filetype::document)}
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
    return filetype::other;
}

QIcon findFileIcon(filetype type)
{
    switch (type)
    {
        case filetype::none:
            Q_ASSERT(!"none is not a valid filetype");
        case filetype::audio:
            return QIcon("icons:ico_filetype_audio.png");
        case filetype::video:
            return QIcon("icons:ico_filetype_video.png");
        case filetype::image:
            return QIcon("icons:ico_filetype_image.png");
        case filetype::text:
            return QIcon("icons:ico_filetype_text.png");
        case filetype::archive:
            return QIcon("icons:ico_filetype_archive.png");
        case filetype::parity:
            return QIcon("icons:ico_filetype_parity.png");
        case filetype::document:
            return QIcon("icons:ico_filetype_document.png");
        case filetype::other:
            return QIcon("icons:ico_filetype_other.png");
    }

    Q_ASSERT(!"wat");

    return QIcon();
}

} // app
