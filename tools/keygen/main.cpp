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

#include <QtCore/QCoreApplication>
#include <QtNetwork/QNetworkReply>
#include <QByteArray>
#include <QString>
#include <iostream>
#include <app/webengine.h>
#include <app/webquery.h>
#include "keygen.h"
#include "secret.h"

bool readYesNo(const char* question)
{
    for (;;)
    {
        std::cout << question;
        int c = std::getchar();
        std::getchar(); // pop the enter
        if (c == 'y' || c == 'Y')
            break;
        else if (c == 'n' || c == 'N')
            return false;
        else
        {
            std::cout << "Excuse me?\n";
            continue;
        }
    }
    return true;
}

QString readInput(const char* text)
{
    std::cout << text;
    std::string s;
    std::getline(std::cin, s);
    return QString::fromUtf8(s.c_str(), s.size());
}

std::ostream& operator<<(std::ostream& out, const QString& s)
{
    const auto& b = s.toUtf8();
    out.write(b.constBegin(), b.size());
    return out;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc != 2)
    {
        std::cout << "needs fingerprint parameter!\n\n";
        return 1;
    }
    using namespace keygen;

    const QString& fingerprint = QString::fromLatin1(argv[1]);
    const QString& keycode     = generate_keycode(fingerprint);

    const QByteArray& bytes = keycode.toLatin1();
    std::cout.write(bytes.constData(), bytes.size());
    std::cout << std::endl;

    if (!readYesNo("\nUpdate the database? Y)es, N)o :"))
        return 0;

    const auto paypalRef   = readInput("\nPaypal reference:");
    const auto contributor = readInput("\nContributor:");
    const auto euro        = readInput("\nAmount in euro:");

    // std::cout << "\nPaypal ref: " << paypalRef;
    // std::cout << "\nContributor: " << contributor;
    // std::cout << "\nEuro amount:" << euro;

    if (!readYesNo("\nProceed with the above information? Y)yes, N)o :"))
        return 0;

    QUrl url("http://www.ensisoft.com/keygen.php");
    url.addQueryItem("fingerprint", fingerprint);
    url.addQueryItem("licensecode", keycode);
    url.addQueryItem("paypalref", paypalRef);
    url.addQueryItem("contributor", contributor);
    url.addQueryItem("euro", euro);
    url.addQueryItem("magic", keygen::MAGIC);

    app::WebEngine web;
    app::WebQuery  query(url);
    query.OnReply = [&](QNetworkReply& reply) {
        const auto all = reply.readAll();
        std::cout.write(all.constData(), all.size());
        app.quit();
    };

    web.submit(query);
    app.exec();

    //std::cout << "All done!";
    return 0;
}
