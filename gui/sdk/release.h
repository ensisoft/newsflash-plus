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

#pragma once

#include <newsflash/warnpush.h>
#  include <QString>
#  include <QtGlobal>
#  include <QDateTime>
#include <newsflash/warnpop.h>
#include "mediatype.h"

namespace sdk
{
    // a media, movie, tv etc release in the usenet
    struct release 
    {
        // the category/mediatype (should be tags really)
        mediatype type;

        // expected/estimated size in bytes.
        // 0 if not known
        quint64 size;    

        // the human readable title for the release
        QString title;

        // link to extra information if applicable
        QString infolink;

        // url for retrieving the nzb
        QString nzburl;

        // url for retrieving information
        QString nfourl;

        // item id
        QString id;

        // publication date in local time zone
        QDateTime pubdate;

        // true if passworded archive
        bool passworded;
    };

} // sdk
