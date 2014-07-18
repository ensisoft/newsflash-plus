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

#include "../datastore.h"
#include "../accounts.h"
#include "../account.h"

int test_main(int, char* argv[])
{
    app::datastore data;

    {
        app::accounts accounts;

        app::account acc {"test"};
        acc.username("user");
        acc.password("pass");
        acc.general_host("news.tcp.com");
        acc.general_port(119);
        acc.secure_host("news.ssl.com");
        acc.secure_port(546);
        acc.enable_general_server(true);
        acc.enable_secure_server(true);
        acc.enable_login(true);
        acc.enable_compression(true);
        acc.enable_pipelining(true);
        acc.quota_type(app::account::quota::fixed);
        acc.connections(5);
        acc.quota_total(8000);
        acc.quota_spent(4000);
        acc.downloads_all_time(1);
        acc.downloads_this_month(2);

        accounts.set(acc);
        accounts.save(data);
        accounts.del(0);

        accounts.load(data);

        acc = accounts.get(0);
        BOOST_REQUIRE(acc.name() == "test");
        BOOST_REQUIRE(acc.username() == "user");
        BOOST_REQUIRE(acc.password() == "pass");
        BOOST_REQUIRE(acc.general_host() == "news.tcp.com");
        BOOST_REQUIRE(acc.general_port() == 119);
        BOOST_REQUIRE(acc.secure_host() == "news.ssl.com");
        BOOST_REQUIRE(acc.secure_port() == 546);
        BOOST_REQUIRE(acc.enable_general_server() == true);
        BOOST_REQUIRE(acc.enable_secure_server() == true);
        BOOST_REQUIRE(acc.enable_login() == true);
        BOOST_REQUIRE(acc.enable_compression() == true);
        BOOST_REQUIRE(acc.enable_pipelining() == true);
        BOOST_REQUIRE(acc.connections() == 5);
        BOOST_REQUIRE(acc.quota_total() == 8000);
        BOOST_REQUIRE(acc.quota_spent() == 4000);
        BOOST_REQUIRE(acc.downloads_all_time() == 1);
        BOOST_REQUIRE(acc.downloads_this_month() == 2);
        BOOST_REQUIRE(acc.quota_type() == app::account::quota::fixed);
    }

    return 0;
}        