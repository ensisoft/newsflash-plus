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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QCoreApplication>
#  include <QFile>
#  include <QEventLoop>
#include "newsflash/warnpop.h"

#include <vector>
#include <list>

#include "app/webengine.h"
#include "app/webquery.h"
#include "test_minimal.h"

void test_basic_get()
{
    // todo:
}

void test_basic_post()
{
    // todo:
}

void test_loop_success(const char* url)
{
    app::WebEngine engine;

    int callbacks = 0;

    for (int i=0; i<10; ++i)
    {
        auto* q = engine.submit(QUrl(url));
        q->OnReply = [&](QNetworkReply& reply) {
            const auto err = reply.error();
            TEST_REQUIRE(err == QNetworkReply::NoError);
            ++callbacks;
        };
    }
    QEventLoop loop;
    QObject::connect(&engine, SIGNAL(allFinished()), &loop, SLOT(quit()));
    loop.exec();

    TEST_REQUIRE(callbacks == 10);
}

void test_loop_error(const char* url)
{
    app::WebEngine engine;

    int callbacks = 0;

    for (int i=0; i<10; ++i)
    {
        auto* q = engine.submit(QUrl(url));
        q->OnReply = [&](QNetworkReply& reply) {
            const auto err = reply.error();
            TEST_REQUIRE(err != QNetworkReply::NoError);
            ++callbacks;
        };
    }

    QEventLoop loop;
    QObject::connect(&engine, SIGNAL(allFinished()), &loop, SLOT(quit()));
    loop.exec();

    TEST_REQUIRE(callbacks == 10);
}

void test_loop_timeout()
{
    app::WebEngine engine;
    engine.setTimeout(5);

    int timeout = 0;

    for (int i=0; i<10; ++i)
    {
        auto* q = engine.submit(QUrl("http://httpbin.org/delay/10"));
        q->OnReply = [&](QNetworkReply& reply) {
            const auto err = reply.error();
            TEST_REQUIRE(err == QNetworkReply::OperationCanceledError);
            ++timeout;
        };
    }
    QEventLoop loop;
    QObject::connect(&engine, SIGNAL(allFinished()), &loop, SLOT(quit()));
    loop.exec();

    TEST_REQUIRE(timeout == 10);
}

int test_main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_loop_success("http://httpbin.org/ip");
    test_loop_success("https://httpbin.org/ip");
    test_loop_error("http://asgsagsaas.foo");
    test_loop_timeout();
    return 0;

}