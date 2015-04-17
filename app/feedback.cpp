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

#define LOGTAG "feedback"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QFile>
#  include <QUrl>
#  include <QFileInfo>
#include <newsflash/warnpop.h>
#include <stdexcept>
#include "feedback.h"
#include "debug.h"
#include "platform.h"
#include "webquery.h"
#include "webengine.h"

// all the data input by the user is sent over HTTP POST/GET query
// to www.ensisoft.com/someapi.php. The UI data is passed 
// in the query parameters. 
// This mechanism helps to a) block spamming b) allows the client to 
// remain simple, i.e. no SMTP configuration is required for the client
//  (or ext<ernal email client) to send the feedback data
// and  c) allows the same data to be inserted into a database
// and integrated with the web site.
//
// see, <cvsroot>www/schema.sql, feedback.php
//
// A script when executed will return an "error code" in the HTTP response body.
// For a list of possible error codes see www/common.php.
//

namespace app
{

Feedback::Feedback() : type_(Type::FeedBack), feeling_(Feeling::Positive)
{
    platform_ = getPlatformName();
    version_  = NEWSFLASH_VERSION;
}

Feedback::~Feedback()
{}

void Feedback::send()
{
    QUrl url("http://www.ensisoft.com/feedback.php");
    url.addQueryItem("name", name_);
    url.addQueryItem("email", email_);
    url.addQueryItem("country", country_);
    url.addQueryItem("version", version_);
    url.addQueryItem("platform", platform_);
    url.addQueryItem("text", text_);

    // these are the legacy values.
    const char* TYPE_NEGATIVE_FEEDBACK = "1";
    const char* TYPE_POSITIVE_FEEDBACK = "2";
    const char* TYPE_NEUTRAL_FEEDBACK  = "3";
    const char* TYPE_BUG_REPORT        = "4";
    const char* TYPE_FEATURE_REQUEST   = "5";
    const char* TYPE_LICENSE_REQUEST   = "6";    

    switch (type_)
    {
        case Type::FeedBack:
            switch (feeling_)
            {
                case Feeling::Positive:
                    url.addQueryItem("type", TYPE_POSITIVE_FEEDBACK);
                    break;
                case Feeling::Neutral:
                    url.addQueryItem("type", TYPE_NEUTRAL_FEEDBACK);
                    break;
                case Feeling::Negative:
                    url.addQueryItem("type", TYPE_NEGATIVE_FEEDBACK);
                    break;
            }
            break;

        case Type::BugReport:
            url.addQueryItem("type", TYPE_BUG_REPORT);
            break;

        case Type::FeatureRequest:
            url.addQueryItem("type", TYPE_FEATURE_REQUEST);
            break;

        case Type::LicenseRequest:
            url.addQueryItem("type", TYPE_LICENSE_REQUEST);
            break;
    }

    auto completion_routine = [&](QNetworkReply& reply) 
    {
        const auto err = reply.error();
        if (err != QNetworkReply::NoError)
        {
            onComplete(Response::NetworkError);
            return;
        }
        // we get a single integer back as the response code
        // and this maps to the values in the response enum
        const auto data = reply.readAll();
        const auto code = QString::fromUtf8(data.constData(), data.size()).toInt();
        DEBUG("Got feedback response code %1", code);

        Q_ASSERT(code >= 0);
        Q_ASSERT(code <= 4);
        onComplete((Response)code);
    };

    if (!attachment_.isEmpty())
    {
        QFile file(attachment_);
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("failed to open attachment");

        auto data = file.readAll();
        auto name = QFileInfo(attachment_).fileName();

        WebQuery query(url, name, data);
        query.OnReply = completion_routine;
        g_web->submit(query);
    }
    else
    {
        WebQuery query(url);
        query.OnReply = completion_routine;
        g_web->submit(query);
    }
}

} // app