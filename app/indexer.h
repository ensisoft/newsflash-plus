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
#include <newsflash/engine/bitflag.h>
#include <vector>
#include "media.h"

class QIODevice;
class QUrl;

namespace app
{
    // Indexer objects know how to prepare QUrl objects for a given
    // Usenet Indexer such as newznab (any site), binsearch.info or nzbindex.nl
    // and how to parse the result data.
    class Indexer
    {
    public:
        enum class Error {
            None,

            // there was an error parsing the content response from the server.
            Content,

            // No such item 
            NoSuchItem,

            // the given credentials were wrong.
            IncorrectCredentials,

            // the account is suspended
            AccountSuspended,

            // the account is not authorized to access the content.
            NoPermission,

            Unknown
        };

        enum class Category {
            Music,
            Movies, 
            Television,
            Console, 
            Apps,
            Porno,
            Other
        };
        using Categories = newsflash::bitflag<Category>;

        struct BasicQuery {
            QString keywords;
            quint32 offset;
            quint32 size;
        };

        struct AdvancedQuery {
            QString keywords;
            quint32 offset;            
            quint32 size;
            Categories categories;
        };

        struct MusicQuery {
            QString album;
            QString track;
            QString year;
            QString artist;
            quint32 offset;            
            quint32 size;
        };

        struct TelevisionQuery {
            QString season;
            QString episode;
            QString keywords;
            quint32 offset;            
            quint32 size;
        };

        virtual ~Indexer() = default;

        // parse the response data from the given IO device.
        // media items parsed from the content are placed into the result vector.
        // finally function returns one of the error values indicating
        // error or success.
        virtual Error parse(QIODevice& io, std::vector<MediaItem>& results) = 0;

        // prepare a search URL
        virtual void prepare(const BasicQuery& query, QUrl& url) = 0;
        virtual void prepare(const AdvancedQuery& query, QUrl& url) = 0;
        virtual void prepare(const MusicQuery& query, QUrl& url) = 0;
        virtual void prepare(const TelevisionQuery& query, QUrl& url) = 0;

        // get the human readable name for the indexing site.
        virtual QString name() const = 0;
    protected:
    private:
    };

} // app