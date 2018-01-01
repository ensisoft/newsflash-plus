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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QString>
#include "newsflash/warnpop.h"

#include <functional>
#include <vector>

#include "engine/bitflag.h"
#include "indexer.h"
#include "rssfeed.h"

namespace app
{
    class WebQuery;

    namespace newznab {

    struct Account {
        QString userid;
        QString apiurl;
        QString apikey;
        QString username;
        QString password;
        QString email;
    };

    struct HostInfo {
        QString strapline;
        QString email;
        QString version;
        QString error;
        QString username;
        QString password;
        QString apikey;
        bool success = false;
    };

    struct ImportList {
        bool success = false;
        QString error;
        std::vector<QString> hosts;
    };

    // Implements a newznab search against some newznab server
    class Search : public app::Indexer
    {
    public:
        Search(const Account& account, app::MediaTypeFlag categories)
        {
            apikey_ = account.apikey;
            apiurl_ = account.apiurl;
            categories_ = categories;
        }

        // Indexer implementation
        virtual Error parse(QIODevice& io, std::vector<MediaItem>& results) override;
        virtual void prepare(const BasicQuery& query, QUrl& url) override;
        virtual void prepare(const AdvancedQuery& query, QUrl& url) override;
        virtual void prepare(const MusicQuery& query, QUrl& url) override;
        virtual void prepare(const TelevisionQuery& query, QUrl& url) override;
        virtual QString name() const override;

    private:
        QString apikey_;
        QString apiurl_;
        app::MediaTypeFlag categories_;
    };

    // implements RSS support against some newznab server
    class RSSFeed : public app::RSSFeed
    {
    public:
        RSSFeed(const Account& account, app::MediaTypeFlag categories)
        {
            userid_ = account.userid;
            apiurl_ = account.apiurl;
            apikey_ = account.apikey;
            categories_ = categories;
        }

        // RSSFeed implementation
        virtual bool parse(QIODevice& io, std::vector<MediaItem>& rss) override;
        virtual void prepare(MainMediaType m, std::vector<QUrl>& urls) override;
        virtual QString name() const override;
    private:
        QString userid_;
        QString apiurl_;
        QString apikey_;
        app::MediaTypeFlag categories_;
    };


    using HostCallback   = std::function<void(const HostInfo&)>;
    using ImportCallback = std::function<void(const ImportList&)>;

    // test the given account
    app::WebQuery* testAccount(const Account& account, const HostCallback& callback);

    // try to register an account with the details
    app::WebQuery* registerAccount(const Account& account, const HostCallback& callback);

    // import a list of "currently known good servers".
    // these each probably require registration before they work
    // properly.
    app::WebQuery* importServerList(const ImportCallback& callback);

    } // namespace newznab

} // app
