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
#include "engine/utility.h"
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

    // try opening non-existing file, expect error
    {
        newsflash::bigfile file;
        std::error_code error;
        file.open("test0.file", newsflash::bigfile::o_no_flags, &error);
        BOOST_REQUIRE(error);
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

    {
        newsflash::bigfile file;
        std::error_code error;
        file.open("test0.file", newsflash::bigfile::o_create, &error);
        BOOST_REQUIRE(file.is_open());
        BOOST_REQUIRE(file.size() == 0);
        BOOST_REQUIRE(error == std::error_code());
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

        std::error_code error;
        file.open(".", newsflash::bigfile::o_no_flags, &error);
        BOOST_REQUIRE(error);

#if defined(LINUX_OS)
        REQUIRE_EXCEPTION(file.open("/dev/mem")); // no permission
        file.open("/dev/mem", newsflash::bigfile::o_no_flags, &error);
        BOOST_REQUIRE(error);
#elif defined(WINDOWS_OS)
        REQUIRE_EXCEPTION(file.open("\\\foobar\file"));
        file.open("\\\foobar\file", newsflash::bigfile::o_no_flags, &error);
        BOOST_REQUIRE(error);
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

        file.close();

        delete_file("test1.file");
    }

    {
        delete_file("test1.file");
        generate_file("test1.file", 512);

        newsflash::bigfile file("test1.file");
        BOOST_REQUIRE(file.read(&empty[0], 512) == 512);
        file.close();

        delete_file("test1.file");
    }
}


void test_large_file()
{
    delete_file("test2.file");

    using big_t = newsflash::bigfile::big_t;

    const char buff[] = "foobar";

    newsflash::bigfile file;
    file.open("test2.file", newsflash::bigfile::o_create);
    file.resize(big_t(0xFFFFFFFFL) + sizeof(buff));
    BOOST_REQUIRE(file.size() == big_t(0xFFFFFFFFL) + sizeof(buff));

    file.seek(0);
    file.write(buff, sizeof(buff));

    file.seek(big_t(0xFFFFFFFFL));
    file.write(buff, sizeof(buff));
    file.close();

    file.open("test2.file", newsflash::bigfile::o_append);
    file.write(buff, sizeof(buff));
    BOOST_REQUIRE(file.size() == big_t(0xffffffffl) + 2 * sizeof(buff));
    file.close();

    char read_buff[sizeof(buff)];

    file.open("test2.file", newsflash::bigfile::o_no_flags);
    file.seek(0);
    file.read(read_buff, sizeof(read_buff));
    BOOST_REQUIRE(!std::memcmp(read_buff, buff, sizeof(buff)));

    std::memset(read_buff, 0, sizeof(read_buff));

    file.seek(big_t(0xffffffffl));
    file.read(read_buff, sizeof(read_buff));
    BOOST_REQUIRE(!std::memcmp(read_buff, buff, sizeof(buff)));

    std::memset(read_buff, 0, sizeof(read_buff));

    file.read(read_buff, sizeof(read_buff));
    BOOST_REQUIRE(!std::memcmp(read_buff, buff, sizeof(buff)));
    BOOST_REQUIRE(file.position() == big_t(0xffffffffl) + 2 * sizeof(buff));

    file.close();

    delete_file("test2.file");
}

void test_huge_file()
{
    delete_file("huge_file_test");

    std::vector<char> buffer(newsflash::MB(512));
    fill_random(&buffer[0], buffer.size());

    {
        newsflash::bigfile file;
        file.open("huge_file_test", newsflash::bigfile::o_create);

        for (int i=0; i<20; ++i)
        {
            file.write(&buffer[0], buffer.size());
        }
    }

    {
        newsflash::bigfile file;
        file.open("huge_file_test", newsflash::bigfile::o_no_flags);
        BOOST_REQUIRE(file.size() == buffer.size() * 20);

        std::vector<char> data;
        data.resize(newsflash::MB(512));
        for (int i=0; i<20; ++i)
        {
            file.read(&data[0], data.size());
            BOOST_REQUIRE(data == buffer);
        }
    }

    delete_file("huge_file_test");
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

void test_static_methods()
{
    // todo:
}

int test_main(int, char* [])
{
    test_file_open();
    test_file_write_read();
    test_unicode_filename();
    test_static_methods();
    test_large_file();
    test_huge_file();
    return 0;
}

