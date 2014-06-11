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

#include <newsflash/config.h>

#include <boost/test/minimal.hpp>

#include <QCoreApplication>
#include <QEventLoop>
#include <QByteArray>
#include <QTextStream>
#include <QBuffer>
#include <cstdio>

#include "../datastore.h"

int test_main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    sdk::datastore datastore;
    BOOST_REQUIRE(!datastore.contains("foo", "bar"));
    datastore.set("foo", "bar", QVariant(1234));
    BOOST_REQUIRE(datastore.contains("foo", "bar"));
    BOOST_REQUIRE(datastore.get("foo", "bar").toInt() == 1234);

    datastore.set("bar", "bar", QVariant("kekeke"));
    BOOST_REQUIRE(datastore.contains("foo", "bar"));
    BOOST_REQUIRE(datastore.contains("bar", "bar"));
    BOOST_REQUIRE(datastore.get("bar", "bar").toString() == "kekeke");

    BOOST_REQUIRE(datastore.get("no", "value", 1234) == 1234);

    QByteArray buffer;
    QBuffer io(&buffer);
    io.open(QIODevice::ReadWrite);

    datastore.clear();
    datastore.set("app", "setting-one", 1234);
    datastore.set("app", "setting-two", "kekeke");
    datastore.set("plugin", "something", 0.556);
    datastore.set("lib", "value", true);
    datastore.save(io);

    // QTextStream out(stdout);
    // out << buffer;

    io.seek(0);
    datastore.clear();
    datastore.load(io);
    BOOST_REQUIRE(datastore.get("app", "setting-one").toInt() == 1234);
    BOOST_REQUIRE(datastore.get("app", "setting-two").toString() == "kekeke");
    BOOST_REQUIRE(datastore.get("lib", "value").toBool() == true);
    return 0;
}