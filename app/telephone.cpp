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

#define LOGTAG "home"

#include "config.h"

#include "warnpush.h"
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QUrl>
#  include <QRegExp>
#include "warnpop.h"

#include "tools/keygen/keygen.h"
#include "telephone.h"
#include "debug.h"
#include "eventlog.h"
#include "platform.h"
#include "format.h"
#include "version.h"
#include "webengine.h"
#include "webquery.h"

namespace app
{

Telephone::Telephone()
{
    DEBUG("Current platform %1", getPlatformName());
}

Telephone::~Telephone()
{}

void Telephone::callhome()
{
    DEBUG("Calling home...");

    const auto& platform    = getPlatformName();
    const auto& fingerprint = keygen::generate_fingerprint();

    QUrl url;
    url.setUrl("http://ensisoft.com/callhome.php");
    url.addQueryItem("version", NEWSFLASH_VERSION);
    url.addQueryItem("platform", platform);
    url.addQueryItem("fingerprint", fingerprint);

    WebQuery query(url);
    query.OnReply = std::bind(&Telephone::onFinished, this,
        std::placeholders::_1);
    g_web->submit(query);
}

void Telephone::onFinished(QNetworkReply& reply)
{
    const auto err = reply.error();
    if (err != QNetworkReply::NoError)
    {
        WARN("Checking for new version failed %1", err);
        emit completed(false, "");
        return;
    }

    QByteArray response = reply.readAll();
    QString latest  = QString::fromUtf8(response.constData(), response.size());
    QString current = NEWSFLASH_VERSION;

    // todo: error code checking coming from the site

    latest.remove(QRegExp("\\n"));
    latest.remove(QRegExp("\\r"));

    const bool newVersion = checkVersionUpdate(NEWSFLASH_VERSION, latest);
    DEBUG("Latest available version %1", latest);
    DEBUG("Have new version? %1", newVersion);

    emit completed(newVersion, latest);
}

} // app
