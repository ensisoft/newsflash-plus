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
#include <chrono>
#include "../filemap.h"
#include "../filebuf.h"
#include "../catalog.h"
#include "unit_test_common.h"


void unit_test_create_new()
{
    using catalog = newsflash::catalog<newsflash::filebuf>;

    delete_file("file");

    // create a new database
    {
        catalog db;
        db.open("file");

        BOOST_REQUIRE(db.article_count() == 0);
        BOOST_REQUIRE(db.article_start() == 0);

        newsflash::article a {};
        a.author  = "John Doe";
        a.subject = "[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)";
        a.bytes   = 1024;
        a.number  = 666;
        a.bits.set(newsflash::article::flags::broken);
        a.bits.set(newsflash::article::flags::binary);
        db.append(a);
        BOOST_REQUIRE(db.article_count() == 1);

        a.author  = "Mickey mouse";
        a.subject = "Mickey and Goofy in Disneyland";
        a.bytes   = 456;
        a.number  = 500;
        a.bits.clear();
        a.bits.set(newsflash::article::flags::downloaded);
        db.append(a);
        BOOST_REQUIRE(db.article_count() == 2);

        a.author  = "foo@acme.com";
        a.subject = "Leiah - Kings and Queens \"Foobar.mp3\" (01/10)";
        a.bytes   = 1024;
        a.number  = 45;
        a.bits.clear();
        db.append(a);
        BOOST_REQUIRE(db.article_count() == 3);

        db.flush();
    }

    // open an existing
    {
        catalog db;
        db.open("file");
        BOOST_REQUIRE(db.article_count() == 3);                

        // iterators
        auto beg = db.begin();
        auto end = db.end();
        BOOST_REQUIRE(beg != end);


        auto a = db.lookup(catalog::offset_t(0));
        BOOST_REQUIRE(a.bits.test(newsflash::article::flags::broken));
        BOOST_REQUIRE(a.bits.test(newsflash::article::flags::binary));
        BOOST_REQUIRE(a.author  == "John Doe");
        BOOST_REQUIRE(a.subject == "[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)");
        BOOST_REQUIRE(a.bytes   == 1024);
        BOOST_REQUIRE(a.number  == 666);

        BOOST_REQUIRE(a == *beg);
        BOOST_REQUIRE(beg->author == "John Doe");
        BOOST_REQUIRE(beg->bytes  == 1024);

        a = db.lookup(catalog::offset_t{a.next()});
        BOOST_REQUIRE(a.bits.test(newsflash::article::flags::downloaded));
        BOOST_REQUIRE(a.author  == "Mickey mouse");
        BOOST_REQUIRE(a.subject == "Mickey and Goofy in Disneyland");
        BOOST_REQUIRE(a.bytes == 456);
        BOOST_REQUIRE(a.number == 500);

        beg++;
        BOOST_REQUIRE(a == *beg);
        BOOST_REQUIRE(beg->author == "Mickey mouse");
        BOOST_REQUIRE(beg->bytes == 456);

        a = db.lookup(catalog::offset_t{a.next()});
        BOOST_REQUIRE(a.author == "foo@acme.com");
        BOOST_REQUIRE(a.number == 45);

        beg++;
        BOOST_REQUIRE(a == *beg);
        BOOST_REQUIRE(beg->author == "foo@acme.com");

        beg++;
        BOOST_REQUIRE(beg == end);
    }

    delete_file("file");
}

void unit_test_performance()
{
    delete_file("file");

    // write
    {
        using catalog = newsflash::catalog<newsflash::filebuf>;

        catalog db;
        db.open("file");

        const auto beg = std::chrono::steady_clock::now();

        newsflash::article a;
        a.author = "John Doe";
        a.subject = "[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)";

        for (std::size_t i=0; i<1000000; ++i)
        {
            db.append(a);
        }

        db.flush();

        const auto end = std::chrono::steady_clock::now();
        const auto diff = end - beg;
        const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        std::cout << "Writing time spent (ms): " << ms.count() << std::endl;


    }

    // read with filebuf
    {
        using catalog = newsflash::catalog<newsflash::filebuf>;

        catalog db;
        db.open("file");

        const auto beg = std::chrono::steady_clock::now();

        newsflash::article a;
        catalog::offset_t key = 0;
        for (std::size_t i=0; i<1000000; ++i)
        {
            a = db.lookup(key);
            key = a.next();
        }

        const auto end = std::chrono::steady_clock::now();
        const auto diff = end - beg;
        const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        std::cout << "Reading with filebuf Time spent (ms): " << ms.count() << std::endl;
    }

    // read with filemap
    {
        using catalog = newsflash::catalog<newsflash::filemap>;

        catalog db;
        db.open("file");

        const auto beg = std::chrono::steady_clock::now();

        newsflash::article a;
        catalog::offset_t key = 0;
        for (std::size_t i=0; i<1000000; ++i)
        {
            a = db.lookup(key);
            key = a.next();
        }

        const auto end = std::chrono::steady_clock::now();
        const auto diff = end - beg;
        const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        std::cout << "Reading with filemap Time spent (ms): " << ms.count() << std::endl;        
    }
    
}

void check_file()
{
    using catalog = newsflash::catalog<newsflash::filebuf>;

    catalog db;
    db.open("vol33417.dat");

    auto beg = db.begin();
    auto end = db.end();
    for (; beg != end; ++beg)
    {
        auto article = *beg;
        std::cout << article.subject << std::endl;
    }

}

int test_main(int, char*[])
{
    unit_test_create_new();
    //check_file();

    //unit_test_performance();

    return 0;
}