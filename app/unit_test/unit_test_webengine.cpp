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

#include <newsflash/config.h>
#include <newsflash/test_minimal.h>

#include <newsflash/warnpush.h>
//#  include <boost/test/minimal.hpp>
#  include <QCoreApplication>
#  include <QFile>
#  include <QEventLoop>
#  include <newsflash/warnpop.h>
#include <vector>
#include <list>
#include "../webengine.h"
#include "../webquery.h"

void test_basic_get()
{
    // todo:
}

void test_basic_post()
{
    // todo:
}

void test_mass_success()
{
    app::WebEngine engine;

    int callbacks = 0;

    for (int i=0; i<100; ++i)
    {
        auto* q = engine.submit(QUrl("http://httpbin.org/ip"));
        q->OnReply = [&](QNetworkReply& reply) {
            const auto err = reply.error();
            TEST_REQUIRE(err == QNetworkReply::NoError);
            ++callbacks;
        };
    }
    QEventLoop loop;
    QObject::connect(&engine, SIGNAL(allFinished()), &loop, SLOT(quit()));
    loop.exec();

    TEST_REQUIRE(callbacks == 100);
}

void test_mass_error()
{
    app::WebEngine engine;

    int callbacks = 0;

    for (int i=0; i<100; ++i)
    {
        auto* q = engine.submit(QUrl("http://asgsagsaas.foo"));
        q->OnReply = [&](QNetworkReply& reply) {
            const auto err = reply.error();
            TEST_REQUIRE(err != QNetworkReply::NoError);
            ++callbacks;
        };
    }

    QEventLoop loop;
    QObject::connect(&engine, SIGNAL(allFinished()), &loop, SLOT(quit()));
    loop.exec();

    TEST_REQUIRE(callbacks == 100);
}

void test_mass_timeout()
{
    app::WebEngine engine;
    engine.setTimeout(5);

    int timeout = 0;
    int success = 0;

    for (int i=0; i<100; ++i)
    {
        if (i % 2)
        {
            auto* q = engine.submit(QUrl("http://httpbin.org/delay/10"));
            q->OnReply = [&](QNetworkReply& reply) {
                const auto err = reply.error();
                TEST_REQUIRE(err == QNetworkReply::OperationCanceledError);
                ++timeout;
            };
        }
        else
        {
            auto* q = engine.submit(QUrl("http://httpbin.org/ip"));
            q->OnReply = [&](QNetworkReply& reply) {
                const auto err = reply.error();
                TEST_REQUIRE(err == QNetworkReply::NoError);
                ++success;
            };
        }
    }    
    QEventLoop loop;
    QObject::connect(&engine, SIGNAL(allFinished()), &loop, SLOT(quit()));
    loop.exec();

    TEST_REQUIRE(timeout == 50);
    TEST_REQUIRE(success == 50);
}

int test_main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_mass_success();
    test_mass_error();
    test_mass_timeout();
    return 0;

}