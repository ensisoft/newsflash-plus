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
#  include <QObject>
#include <newsflash/warnpop.h>
#include <functional>
#include <vector>
#include "indexer.h"

namespace app
{
    class WebQuery;

    class Newznab : public QObject, public Indexer
    {
        Q_OBJECT

    public:
        struct Account {
            QString apiurl;
            QString apikey;
            QString username;
            QString password;
            QString email;
        };

        virtual Error parse(QIODevice& io, std::vector<MediaItem>& results) override;
        virtual void prepare(const BasicQuery& query, QUrl& url) override;
        virtual void prepare(const AdvancedQuery& query, QUrl& url) override;
        virtual void prepare(const MusicQuery& query, QUrl& url) override;
        virtual void prepare(const TelevisionQuery& query, QUrl& url) override;
        virtual QString name() const override;

        void setAccount(const Account& acc);

        struct HostInfo {
            QString strapline;
            QString email;
            QString version;
            QString error;
            QString username;
            QString password;
            QString apikey;
            bool success;
        };
        struct ImportList {
            bool success;
            QString error;
            std::vector<QString> hosts;
        };

        using HostCallback = std::function<void(const HostInfo&)>;
        using ImportCallback = std::function<void(const ImportList&)>;

        static 
        WebQuery* apiTest(const Account& acc, HostCallback cb);

        static
        WebQuery* apiRegisterUser(const Account& acc, HostCallback cb);

        static
        WebQuery* importList(ImportCallback cb);
    private:
        QString apikey_;
        QString apiurl_;
    };

} // app