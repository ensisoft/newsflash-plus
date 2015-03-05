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
#  include <QUrl>
#include <newsflash/warnpop.h>
#include <vector>
#include "media.h"

class QIODevice;
class QUrl;

namespace app
{
    // RSS feed processing interface
    class RSSFeed
    {
    public:
        enum class Error {
            // operation completed succesfully.
            None,

            // there was an error in the content data.
            ContentXml,

            // no such item 
            NoSuchItem,

            // the provided credentials were not correct.
            IncorrectCredentials,

            // account is suspended.
            AccountSuspended,

            // the account doesnt have permission to access this content.
            NoPermission
        };

        struct params {
            QString user;
            QString key;
            int feedsize;
        };
        RSSFeed() : enabled_(true)
        {}

        virtual ~RSSFeed() = default;

        // parse the RSS feed coming from the IO device and store the 
        // parsed media items into the vector.
        virtual bool parse(QIODevice& io, std::vector<MediaItem>& rss) = 0;

        // prepare available URLs to retrieve the RSS feed for the matching media type.
        virtual void prepare(MediaType m, std::vector<QUrl>& urls) = 0;

        // get site URL
        virtual QString site() const = 0;

        // get site name
        virtual QString name() const = 0;

        virtual void setParams(const params& p) {}

        virtual void enable(bool on_off)
        { enabled_ = on_off; }

        virtual bool isEnabled() const 
        { return enabled_; }
    protected:
        bool enabled_;
    private:
    };

} // app