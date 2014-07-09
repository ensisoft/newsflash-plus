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

#include <newsflash/config.h>

#include <newsflash/sdk/request.h>
#include <newsflash/sdk/rssfeed.h>

#include <newsflash/warnpush.h>
#  include <QDateTime>
#  include <QString>
#include <newsflash/warnpop.h>

#include <vector>

namespace rss
{
    class nzbs : public sdk::rssfeed
    {
    public:
        nzbs();
       ~nzbs();

        virtual bool parse(QIODevice& io, std::vector<item>& rss) const override;

        virtual void prepare(sdk::category cat, std::vector<QUrl>& urls) const override;

        virtual bool set_params(const QVariantMap& params);

        virtual QString site() const override;
        virtual QString name() const override;
    private:
        QString userid_;
        QString apikey_;
        int feedsize_;
    };

} // rss
