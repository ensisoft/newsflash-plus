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
#  include <boost/test/minimal.hpp>
#include "newsflash/warnpop.h"

#include <chrono>

#include "engine/filemap.h"
#include "engine/filebuf.h"
#include "engine/catalog.h"
#include "engine/filetype.h"
#include "unit_test_common.h"


void unit_test_create_new()
{
    using catalog  = newsflash::catalog<newsflash::filebuf>;
    using article  = newsflash::article<newsflash::filebuf>;
    using fileflag = newsflash::fileflag;

    delete_file("file");

    // create a new database
    {
        // don't create on the stack since the header is large.'
        auto  db = std::make_unique<catalog>();
        db->open("file");

        BOOST_REQUIRE(db->size() == 0);

        article a;
        a.set_author("John Doe");
        a.set_subject("[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)");
        a.set_bytes(1024);
        a.set_number(666);
        a.set_bits(fileflag::broken, true);
        db->append(a);
        BOOST_REQUIRE(db->size() == 1);

        a.clear();
        a.set_author("Mickey Mouse");
        a.set_subject("Mickey and Goofy in Disneyland");
        a.set_bits(fileflag::downloaded, true);
        a.set_bytes(456);
        a.set_number(500);
        db->append(a);
        BOOST_REQUIRE(db->size() == 2);

        a.clear();
        a.set_subject("Leiah - Kings and Queens \"Foobar.mp3\" (01/10)");
        a.set_author("foo@acme.com");
        a.set_number(45);
        db->append(a);
        BOOST_REQUIRE(db->size() == 3);

        db->flush();
    }

    // open an existing
    {
        auto db = std::make_unique<catalog>();
        db->open("file");
        BOOST_REQUIRE(db->size() == 3);

        // iterators
        auto beg = db->begin();
        auto end = db->end();
        BOOST_REQUIRE(beg != end);

        catalog::offset_t offset(0);

        auto a = db->load(offset);
        BOOST_REQUIRE(a.test(fileflag::broken));
        BOOST_REQUIRE(a.author()  == "John Doe");
        BOOST_REQUIRE(a.subject() == "[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)");
        BOOST_REQUIRE(a.bytes()   == 1024);
        BOOST_REQUIRE(a.number()  == 666);

        BOOST_REQUIRE(beg->author() == "John Doe");
        BOOST_REQUIRE(beg->bytes()  == 1024);

        offset += a.size_on_disk();
        a = db->load(offset);
        BOOST_REQUIRE(a.test(fileflag::downloaded));
        BOOST_REQUIRE(a.author()  == "Mickey Mouse");
        BOOST_REQUIRE(a.subject() == "Mickey and Goofy in Disneyland");
        BOOST_REQUIRE(a.bytes() == 456);
        BOOST_REQUIRE(a.number() == 500);

        beg++;
        BOOST_REQUIRE(beg->author() == "Mickey Mouse");
        BOOST_REQUIRE(beg->bytes() == 456);

        offset += a.size_on_disk();
        a = db->load(offset);
        BOOST_REQUIRE(a.author() == "foo@acme.com");
        BOOST_REQUIRE(a.subject() == "Leiah - Kings and Queens \"Foobar.mp3\" (01/10)");
        BOOST_REQUIRE(a.number() == 45);

        beg++;
        BOOST_REQUIRE(beg->author() == "foo@acme.com");
        BOOST_REQUIRE(beg->subject() == "Leiah - Kings and Queens \"Foobar.mp3\" (01/10)");

        beg++;
        BOOST_REQUIRE(beg == end);
    }

    // open again with different backend
    {
        using catalog = newsflash::catalog<newsflash::filemap>;
        using article = newsflash::catalog<newsflash::filemap>;

        auto db = std::make_unique<catalog>();
        db->open("file");
        BOOST_REQUIRE(db->size() == 3);

        catalog::offset_t offset(0);

        auto a = db->load(offset);
        BOOST_REQUIRE(a.test(fileflag::broken));
        BOOST_REQUIRE(a.author() == "John Doe");
        BOOST_REQUIRE(a.subject() == "[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)");
        BOOST_REQUIRE(a.bytes() == 1024);
        BOOST_REQUIRE(a.number() == 666);

        offset += a.size_on_disk();
        a = db->load(offset);
        BOOST_REQUIRE(a.test(fileflag::downloaded));
        BOOST_REQUIRE(a.author() == "Mickey Mouse");
        BOOST_REQUIRE(a.subject() == "Mickey and Goofy in Disneyland");
        BOOST_REQUIRE(a.bytes() == 456);
        BOOST_REQUIRE(a.number() == 500);

        offset += a.size_on_disk();
        a = db->load(offset);
        BOOST_REQUIRE(a.author() == "foo@acme.com");
        BOOST_REQUIRE(a.subject() == "Leiah - Kings and Queens \"Foobar.mp3\" (01/10)");
        BOOST_REQUIRE(a.number() == 45);

    }

    // open from a snapshot
    {
        using catalog = newsflash::catalog<newsflash::filemap>;
        using article = newsflash::catalog<newsflash::filemap>;

        auto cat = std::make_unique<catalog>();
        cat->open("file");
        BOOST_REQUIRE(cat->size() == 3);

        auto snapshot = cat->snapshot();

        auto db = std::make_unique<catalog>();
        db->open(*snapshot);

        catalog::offset_t offset(0);

        auto a = db->load(offset);
        BOOST_REQUIRE(a.test(fileflag::broken));
        BOOST_REQUIRE(a.author() == "John Doe");
        BOOST_REQUIRE(a.subject() == "[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)");
        BOOST_REQUIRE(a.bytes() == 1024);
        BOOST_REQUIRE(a.number() == 666);

        offset += a.size_on_disk();
        a = db->load(offset);
        BOOST_REQUIRE(a.test(fileflag::downloaded));
        BOOST_REQUIRE(a.author() == "Mickey Mouse");
        BOOST_REQUIRE(a.subject() == "Mickey and Goofy in Disneyland");
        BOOST_REQUIRE(a.bytes() == 456);
        BOOST_REQUIRE(a.number() == 500);

        offset += a.size_on_disk();
        a = db->load(offset);
        BOOST_REQUIRE(a.author() == "foo@acme.com");
        BOOST_REQUIRE(a.subject() == "Leiah - Kings and Queens \"Foobar.mp3\" (01/10)");
        BOOST_REQUIRE(a.number() == 45);
    }

    delete_file("file");
}

void unit_test_performance()
{
    // delete_file("file");

    // // write
    // {
    //     using catalog = newsflash::catalog<newsflash::filebuf>;

    //     catalog db;
    //     db.open("file");

    //     const auto beg = std::chrono::steady_clock::now();

    //     newsflash::article a;
    //     a.author = "John Doe";
    //     a.subject = "[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)";

    //     for (std::size_t i=0; i<1000000; ++i)
    //     {
    //         db.append(a);
    //     }

    //     db.flush();

    //     const auto end = std::chrono::steady_clock::now();
    //     const auto diff = end - beg;
    //     const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    //     std::cout << "Writing time spent (ms): " << ms.count() << std::endl;


    // }

    // // read with filebuf
    // {
    //     using catalog = newsflash::catalog<newsflash::filebuf>;

    //     catalog db;
    //     db.open("file");

    //     const auto beg = std::chrono::steady_clock::now();

    //     newsflash::article a;
    //     catalog::offset_t key = 0;
    //     for (std::size_t i=0; i<1000000; ++i)
    //     {
    //         a = db.lookup(key);
    //         key = a.next();
    //     }

    //     const auto end = std::chrono::steady_clock::now();
    //     const auto diff = end - beg;
    //     const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    //     std::cout << "Reading with filebuf Time spent (ms): " << ms.count() << std::endl;
    // }

    // // read with filemap
    // {
    //     using catalog = newsflash::catalog<newsflash::filemap>;

    //     catalog db;
    //     db.open("file");

    //     const auto beg = std::chrono::steady_clock::now();

    //     newsflash::article a;
    //     catalog::offset_t key = 0;
    //     for (std::size_t i=0; i<1000000; ++i)
    //     {
    //         a = db.lookup(key);
    //         key = a.next();
    //     }

    //     const auto end = std::chrono::steady_clock::now();
    //     const auto diff = end - beg;
    //     const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    //     std::cout << "Reading with filemap Time spent (ms): " << ms.count() << std::endl;
    // }

}

void check_file()
{
    // using catalog = newsflash::catalog<newsflash::filebuf>;

    // catalog db;
    // db.open("vol33417.dat");

    // auto beg = db.begin();
    // auto end = db.end();
    // for (; beg != end; ++beg)
    // {
    //     auto article = *beg;
    //     std::cout << article.subject << std::endl;
    // }

}

int test_main(int, char*[])
{
    unit_test_create_new();
    //check_file();

    //unit_test_performance();

    return 0;
}