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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QString>
#include <newsflash/warnpop.h>

#include "rssfeed.h"

namespace app
{
    // nzbs aka. http://nzbs.org provides nice RSS feeds but requires registration.
    class Nzbs : public RSSFeed
    {
    public:
        virtual bool parse(QIODevice& io, std::vector<MediaItem>& rss) override;

        virtual void prepare(MediaType m, std::vector<QUrl>& urls) override;

        virtual QString site() const override
        { return "http://nzbs.org"; }

        virtual QString name() const override
        { return "nzbs"; }

        virtual void setParams(const params& p) override
        {
            userid_ = p.user;
            apikey_ = p.key;
        }
    private:
        QString userid_;
        QString apikey_;
    };
} // app