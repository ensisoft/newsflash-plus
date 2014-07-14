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

QString filepattern(filetype type)
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
            return ".txt | .nfo | .sfv";
        case filetype::log:
            return ".log";
        case filetype::archive:
            return ".zip | .rar | .r[0-9]{1,3} | .7z";
        case filetype::parity:
            return ".par | par2";
        case filetype::document:
            return ".doc | .chm | .pdf";
        case filetype::other:
            return ".*";
    }
    return "";
}

QString str(filetype type)
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
        case filetype::log:
            return "Log";
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

filetype find_filetype(const QString& filename)
{
    static const QString patterns[] = {
        filepattern(filetype::audio),
        filepattern(filetype::video),
        filepattern(filetype::image),
        filepattern(filetype::text),
        filepattern(filetype::log),
        filepattern(filetype::archive),
        filepattern(filetype::parity),
        filepattern(filetype::document),
        filepattern(filetype::other)
    };

    static_assert((int)filetype::none == 0, 
        "none should be the first filetype and actual types start at 1 (see the cast in the tokens loop");

    for (const auto& pattern : patterns)
    {
        const auto& tokens = pattern.split("|");
        for (int i=0; i<tokens.size(); ++i)
        {
            // match the end of the filestring only
            QRegExp regex(QString("\\%1$").arg(tokens[i].simplified()));
            regex.setCaseSensitivity(Qt::CaseInsensitive);
            if (regex.indexIn(filename) != -1)
                return static_cast<filetype>(i+1);
        }
    }
    return filetype::none;
}

QIcon iconify(filetype type)
{
    switch (type)
    {
        case filetype::none:
            Q_ASSERT(!"none is not a valid filetype");
        case filetype::audio:
            return QIcon(":/resource/16x16_ico_png/ico_audio.png");
        case filetype::video:
            return QIcon(":/resource/16x16_ico_png/ico_video.png");
        case filetype::image:
            return QIcon(":/resource/16x16_ico_png/ico_image.png");
        case filetype::text:
            return QIcon(":/resource/16x16_ico_png/ico_text.png");
        case filetype::log:
            return QIcon(":/resource/16x16_ico_png/ico_log.png");
        case filetype::archive:
            return QIcon(":/resource/16x16_ico_png/ico_archive.png");
        case filetype::parity:
            return QIcon(":/resource/16x16_ico_png/ico_parity.png");
        case filetype::document:
            return QIcon(":/resource/16x16_ico_png/ico_document.png");
        case filetype::other:
            return QIcon(":/resource/16x16_ico_png/ico_document.png");
    }

    return QIcon();
}

} // app
