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

#include <newsflash/warnpop.h>
#include "rssfeed.h"

namespace app
{
    // womble, aka. newshost.co.za provides simple and hassle free RSS feed for usenet. 
    // also no registration or user credentials are required.
    class Womble : public RSSFeed
    {
    public:
        virtual bool parse(QIODevice& io, std::vector<MediaItem>& rss) override;

        // user authentication is not required.
        virtual void prepare(MediaType m, std::vector<QUrl>& urls) override;

        virtual QString site() const override
        { return "http://newshost.co.za"; }

        virtual QString name() const override
        { return "womble"; }
    protected:
    private:
    };
} // app
