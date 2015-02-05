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

#define LOGTAG "feedback"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QFile>
#  include <QUrl>
#  include <QFileInfo>
#include <newsflash/warnpop.h>
#include <stdexcept>
#include "feedback.h"
#include "format.h"
#include "debug.h"

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

Feedback::Feedback()
{
    net_ = g_net->getSubmissionContext();
}

Feedback::~Feedback()
{}

void Feedback::send(const Message& m)
{
    QUrl url("http://www.ensisoft.com/feedback.php");
    url.addQueryItem("name", m.name);
    url.addQueryItem("email", m.email);
    url.addQueryItem("country", m.country);
    url.addQueryItem("version", m.version);
    url.addQueryItem("platform", m.platform);
    url.addQueryItem("text", m.text);

    // these are the legacy values.
    const char* TYPE_NEGATIVE_FEEDBACK = "1";
    const char* TYPE_POSITIVE_FEEDBACK = "2";
    const char* TYPE_NEUTRAL_FEEDBACK  = "3";
    const char* TYPE_BUG_REPORT        = "4";
    const char* TYPE_FEATURE_REQUEST   = "5";
    const char* TYPE_LICENSE_REQUEST   = "6";    

    switch (m.type)
    {
        case type::feedback:
            switch (m.feeling)
            {
                case feeling::positive:
                    url.addQueryItem("type", TYPE_POSITIVE_FEEDBACK);
                    break;
                case feeling::neutral:
                    url.addQueryItem("type", TYPE_NEUTRAL_FEEDBACK);
                    break;
                case feeling::negative:
                    url.addQueryItem("type", TYPE_NEGATIVE_FEEDBACK);
                    break;
            }
            break;

        case type::bugreport:
            url.addQueryItem("type", TYPE_BUG_REPORT);
            break;

        case type::request_feature:
            url.addQueryItem("type", TYPE_FEATURE_REQUEST);
            break;

        case type::request_license:
            url.addQueryItem("type", TYPE_LICENSE_REQUEST);
            break;
    }

    auto completion_routine = [&](QNetworkReply& reply) 
    {
        const auto err = reply.error();
        if (err != QNetworkReply::NoError)
        {
            on_complete(response::network_error);
            return;
        }
        // we get a single integer back as the response code
        // and this maps to the values in the response enum
        const auto data = reply.readAll();
        const auto code = QString::fromUtf8(data.constData(), data.size()).toInt();

        DEBUG(str("Got feedback response code _1", code));

        Q_ASSERT(code >= 0);
        Q_ASSERT(code <= 4);
        on_complete((response)code);
    };

    if (!m.attachment.isEmpty())
    {
        QFile file(m.attachment);
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("failed to open attachment");

        NetworkManager::Attachment attachment;
        attachment.name = QFileInfo(m.attachment).fileName();
        attachment.data = file.readAll();

        g_net->submit(std::move(completion_routine), net_, url, attachment);
    }
    else
    {
        g_net->submit(std::move(completion_routine), net_, url);
    }
}

} // app