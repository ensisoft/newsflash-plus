// Copyright (c) 2010-2013 Sami Väisänen, Ensisoft 
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

// $Id: unit_test_file.cpp,v 1.7 2010/02/25 13:12:40 svaisane Exp $

#include <boost/filesystem/operations.hpp>
#include <boost/test/minimal.hpp>
#include <iostream>
#include "unit_test_common.h"
#include "../bigfile.h"

namespace fs = boost::filesystem;

/*
 * Synopsis: Verify basic file functionality, creating, opening, writing and reading.
 *
 * Expected: No errors
 */
void test_basic_file_ops()
{
    delete_file("test0.file");

    BOOST_REQUIRE(engine::bigfile::size("test0.file").first);
    BOOST_REQUIRE(engine::bigfile::resize("test0.file", 100));

    // try opening non-existing file
    {
        engine::bigfile file;
        BOOST_REQUIRE(file.open("test0.file"));
        BOOST_REQUIRE(file.is_open() == false);
    }

    // try opening an existing file
    {
        generate_file("test0.file", 1024);

        BOOST_REQUIRE(engine::bigfile::size("test0.file").second == 1024);
        engine::bigfile::resize("test0.file", 512);
        BOOST_REQUIRE(engine::bigfile::size("test0.file").second == 512);
        engine::bigfile::resize("test0.file", 0);
        BOOST_REQUIRE(engine::bigfile::size("test0.file").second == 0);        
        engine::bigfile::resize("test0.file", 1234);
        BOOST_REQUIRE(engine::bigfile::size("test0.file").second == 1234);                

        engine::bigfile file;

        BOOST_REQUIRE(!file.open("test0.file"));
        BOOST_REQUIRE(file.is_open());
        BOOST_REQUIRE(file.position() == 0);

        delete_file("test0.file");

    }

    // try opening objects that cant be opened
    {
        engine::bigfile file;

        BOOST_REQUIRE(file.open("."));
        BOOST_REQUIRE(!file.is_open());
#if defined(LINUX_OS)
        BOOST_REQUIRE(file.open("/dev/mem")); // no permission 
//        REQUIRE_EXCEPTION(engine::bigfile::size("/dev/mem"));
//        REQUIRE_EXCEPTION(engine::bigfile::resize("/dev/mem", 100));
#elif defined(WINDOWS_OS)
        BOOST_REQUIRE(file.open("\\\foobar\file"));
#endif
    }

    // try open the file for appending, file doesn't exist it's created
    {

        delete_file("test0.file");

        engine::bigfile file;

        BOOST_REQUIRE(!file.append("test0.file"));
        BOOST_REQUIRE(file.is_open());

        const char* data = "one two three";
        file.write(data, strlen(data));
        file.close();
        
        // open again, 
        BOOST_REQUIRE(!file.append("test0.file"));

        const char* more = "five six seven";
        file.write(more, strlen(more));
        file.close();

        // check that the data was appended        
        const auto vec = read_file_contents("test0.file");
        BOOST_REQUIRE(strcmp(vec.data(), "one two threefive six seven"));

        delete_file("test0.file");        
    }

    // create file
    {
        delete_file("test0.file");        

        // file doesn't exist, its created
        engine::bigfile file;

        BOOST_REQUIRE(!file.create("test0.file"));
        BOOST_REQUIRE(file.is_open());
        BOOST_REQUIRE(engine::bigfile::size("test0.file").second == 0);        
        file.close();

        generate_file("test0.file", 1024);

        // open existing file and truncate it's contents
        BOOST_REQUIRE(!file.create("test0.file"));
        BOOST_REQUIRE(engine::bigfile::size("test0.file").second == 0);

        delete_file("test0.file");                
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

    delete_file("test1.file");

    {
        engine::bigfile file;

        BOOST_REQUIRE(!file.create("test1.file"));

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

        engine::bigfile file;
        BOOST_REQUIRE(!file.open("test1.file"));

        BOOST_REQUIRE(file.read(&empty[0], 512) == 512);

        delete_file("test1.file");
    }
}


void test_large_file()
{
    delete_file("test2.file");

    engine::bigfile file;
    file.create("test2.file");

    using big_t = engine::bigfile::big_t;

    engine::bigfile::resize("test2.file", big_t(0xffffffffL) + 1);
    BOOST_REQUIRE(engine::bigfile::size("test2.file").second == big_t(0xffffffffL) + 1);

    const char buff[] = "foobar";

    file.seek(0);
    file.write(buff, sizeof(buff));

    file.seek(big_t(0xFFFFFFFFL));
    file.write(buff, 1);
    file.close();

    file.append("test2.file");
    file.write(buff, sizeof(buff));

    BOOST_REQUIRE(engine::bigfile::size("test2.file").second == big_t(0xffffffffL) + sizeof(buff));

    delete_file("test2.file");    
}

void test_unicode_filename()
{
#if defined(WINDOWS_OS)
    // わたしわさみ    
    //const char* utf8 = u8"\u308f\u305f\u3057\u308f\u3055\u307f";
    const char utf8[] = {0xe3, 0x82, 0x8f, 0xe3, 0x81, 0x9f, 0xe3, 0x81, 0x97, 0xe3, 0x82, 0x8f, 0xe3, 0x81, 0x95, 0xe3, 0x81, 0xbf, 0};

    engine::bigfile file;

    BOOST_REQUIRE(file.create(utf8) == std::error_code());
    BOOST_REQUIRE(file.is_open());
    file.close();
    BOOST_REQUIRE(!engine::bigfile::erase(utf8));

    BOOST_REQUIRE(file.append(utf8) == std::error_code());
    BOOST_REQUIRE(file.is_open());
    file.close();
    BOOST_REQUIRE(!engine::bigfile::erase(utf8));

#endif
}

void print_err(const std::error_code& err)
{
    std::cout << "\nValue: " << err.value() << std::endl
              << "Message: " << err.message() << std::endl
              << "Category: " << err.category().name() << std::endl;
}

void test_error_codes()
{
    engine::bigfile file;

    // no such file
    auto err = file.open("nosuchfile");
    print_err(err);
    BOOST_REQUIRE(err == std::errc::no_such_file_or_directory);

#if defined(LINUX_OS)

    print_err(file.open("/dev/mem")); // no permission 

#elif defined(WINDOWS_OS)

    print_err(file.open("\\\foobar\\file"));
    
#endif

    
}

int test_main(int, char* [])
{
    test_basic_file_ops();
    test_file_write_read();
    test_unicode_filename();

    test_error_codes();

    return 0;
}

