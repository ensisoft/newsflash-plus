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
#  include <boost/filesystem/operations.hpp>
#  include <boost/test/minimal.hpp>
#include "newsflash/warnpop.h"

#include <iostream>

#include "engine/bigfile.h"
#include "unit_test_common.h"

namespace fs = boost::filesystem;

/*
 * Synopsis: Verify basic file open functioality
 *
 * Expected: No errors
 */
void test_file_open()
{
    delete_file("test0.file");

    // try opening non-existing file
    {
        newsflash::bigfile file;
        REQUIRE_EXCEPTION(file.open("test0.file"));
        BOOST_REQUIRE(file.is_open() == false);
    }

    // create file
    {
        newsflash::bigfile file;

        file.open("test0.file", newsflash::bigfile::o_create);
        BOOST_REQUIRE(file.is_open());
        BOOST_REQUIRE(file.size() == 0);
        file.close();

        delete_file("test0.file");
    }

    // try opening an existing file
    {
        generate_file("test0.file", 1024);

        newsflash::bigfile file;
        file.open("test0.file");
        BOOST_REQUIRE(file.is_open());
        BOOST_REQUIRE(file.position() == 0);
        BOOST_REQUIRE(file.size() == 1024);
        file.close();

        delete_file("test0.file");
    }

    // try opening an existing file and truncate it's contents
    {
        generate_file("test0.file", 1024);

        newsflash::bigfile file;
        file.open("test0.file", newsflash::bigfile::o_truncate);
        BOOST_REQUIRE(file.size() == 0);
        BOOST_REQUIRE(file.is_open());
        file.close();

        delete_file("test0.file");
    }

    // try opening objects that cant be opened
    {
        newsflash::bigfile file;

        REQUIRE_EXCEPTION(file.open("."));

#if defined(LINUX_OS)
        REQUIRE_EXCEPTION(file.open("/dev/mem")); // no permission
#elif defined(WINDOWS_OS)
        REQUIRE_EXCEPTION(file.open("\\\foobar\file"));
#endif
    }

    delete_file("test0.file");
}

void test_file_write_read()
{
    std::vector<char> buff1;
    buff1.resize(1024);

    std::vector<char> buff2;
    buff2.resize(512);

    std::vector<char> empty;
    empty.resize(buff1.size() + buff2.size());

    fill_random(&buff1[0], buff1.size());
    fill_random(&buff2[0], buff2.size());

    {

        delete_file("test1.file");

        newsflash::bigfile file("test1.file", newsflash::bigfile::o_create);
        file.write(&buff1[0], buff1.size());

        BOOST_REQUIRE(file.position() == buff1.size());

        file.seek(0);
        file.read(&empty[0], buff1.size());

        BOOST_REQUIRE(!std::memcmp(&empty[0], &buff1[0], buff1.size()));
        BOOST_REQUIRE(file.position() == buff1.size());

        file.seek(0);
        file.write(&buff2[0], buff2.size());

        BOOST_REQUIRE(file.position() == buff2.size());

        delete_file("test1.file");
    }

    {
        delete_file("test1.file");
        generate_file("test1.file", 512);

        newsflash::bigfile file("test1.file");
        BOOST_REQUIRE(file.read(&empty[0], 512) == 512);

        delete_file("test1.file");
    }
}


void test_large_file()
{
    delete_file("test2.file");

    newsflash::bigfile file;
    file.open("test2.file", true);

    using big_t = newsflash::bigfile::big_t;

    newsflash::bigfile::resize("test2.file", big_t(0xffffffffL) + 1);
    BOOST_REQUIRE(newsflash::bigfile::size("test2.file").second == big_t(0xffffffffL) + 1);

    const char buff[] = "foobar";

    file.seek(0);
    file.write(buff, sizeof(buff));

    file.seek(big_t(0xFFFFFFFFL));
    file.write(buff, 1);
    file.close();

    file.open("test2.file", newsflash::bigfile::o_append);
    file.write(buff, sizeof(buff));

    BOOST_REQUIRE(newsflash::bigfile::size("test2.file").second == big_t(0xffffffffL) + sizeof(buff));

    delete_file("test2.file");
}

void test_unicode_filename()
{
#if defined(WINDOWS_OS)
    // わたしわさみ
    //const char* utf8 = u8"\u308f\u305f\u3057\u308f\u3055\u307f";
    const char utf8[] = {0xe3, 0x82, 0x8f, 0xe3, 0x81, 0x9f, 0xe3, 0x81, 0x97, 0xe3, 0x82, 0x8f, 0xe3, 0x81, 0x95, 0xe3, 0x81, 0xbf, 0};

    newsflash::bigfile file;
    file.open(utf8, newsflash::bigfile::o_create);
    file.close();
    BOOST_REQUIRE(!newsflash::bigfile::erase(utf8));
#endif
}

void print_err(const std::error_code& err)
{
    std::cout << "\nValue: " << err.value() << std::endl
              << "Message: " << err.message() << std::endl
              << "Category: " << err.category().name() << std::endl;
}

// void test_error_codes()
// {
//     newsflash::bigfile file;

//     // no such file
//     auto err = file.open("nosuchfile");
//     print_err(err);
//     BOOST_REQUIRE(err == std::errc::no_such_file_or_directory);

// #if defined(LINUX_OS)
//     print_err(file.open("/dev/mem")); // no permission
// #elif defined(WINDOWS_OS)
//     print_err(file.open("\\\foobar\\file"));
// #endif

// }

void test_static_methods()
{
    // todo:
}

int test_main(int, char* [])
{
    test_file_open();
    test_file_write_read();
    test_unicode_filename();
    //test_error_codes();
    test_static_methods();

    return 0;
}

