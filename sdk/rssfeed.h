// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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
#  include <QDateTime>
#  include <QtGlobal>
#  include <QUrl>
#include <newsflash/warnpop.h>

#include <vector>
#include "category.h"
#include "plugin.h"

class QIODevice;

namespace sdk
{
    class rssfeed 
    {
    public:
        enum {
            version = 1
        };

        // an rss feed item parsed from the rss stream
        struct item {
            category    cat;
            QString     title;
            QString     id;
            QString     nzb;
            QDateTime   pubdate;
            quint64     size;
            bool password;
        };

        struct settings {
            QString username;
            QString password;
        };

        virtual ~rssfeed() = default;

        virtual void configure(settings settings) {}

        // parse the contents of the RSS data and insert the items into the vector.
        // returns true if parsing was succesful, otherwise false.
        virtual bool parse(QIODevice& io, std::vector<item>& rss) const = 0;

        // prepare rss stream urls for matching media type.
        virtual void prepare(category cat, std::vector<QUrl>& urls) const = 0;

        // return the site url
        virtual QString site() const = 0;

    protected:
    private:
    };

    typedef rssfeed* (*fp_create_rssfeed)(int);

} // sdk

PLUGIN_API sdk::rssfeed* create_rssfeed(int version);