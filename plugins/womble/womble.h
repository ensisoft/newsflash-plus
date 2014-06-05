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

#include <newsflash/sdk/plugin.h>
#include <newsflash/warnpush.h>
#  include <QString>
#include <newsflash/warnpop.h>

namespace womble
{
    class plugin : public sdk::plugin
    {
    public:
        plugin(sdk::newsflash* host);
       ~plugin();
                
        // generate request objects for womble RSS feeds
        // in the given category
        QList<sdk::request*> get_rss(sdk::mediatype media);
        
        // generate a request to download NZB from womble
        sdk::request* get_nzb(const QString& id, const QString& url);

        // get plugin name
        QString name() const;
        
        QString host() const;

        // get plugin features
        sdk::bitflag_t features() const;
    private:
        sdk::newsflash* host_;
    };

} // womble
