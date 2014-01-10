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

    REQUIRE_EXCEPTION(newsflash::bigfile::size("test0.file"));
    REQUIRE_EXCEPTION(newsflash::bigfile::resize("test0.file", 100));

    // try opening non-existing file
    {
        newsflash::bigfile file;
        BOOST_REQUIRE(file.open("test0.file"));
        BOOST_REQUIRE(file.is_open() == false);
    }

    // try opening an existing file
    {
        generate_file("test0.file", 1024);

        BOOST_REQUIRE(newsflash::bigfile::size("test0.file") == 1024);
        newsflash::bigfile::resize("test0.file", 512);
        BOOST_REQUIRE(newsflash::bigfile::size("test0.file") == 512);
        newsflash::bigfile::resize("test0.file", 0);
        BOOST_REQUIRE(newsflash::bigfile::size("test0.file") == 0);        
        newsflash::bigfile::resize("test0.file", 1234);
        BOOST_REQUIRE(newsflash::bigfile::size("test0.file") == 1234);                

        newsflash::bigfile file;

        BOOST_REQUIRE(!file.open("test0.file"));
        BOOST_REQUIRE(file.is_open());
        BOOST_REQUIRE(file.position() == 0);

        delete_file("test0.file");

    }

    // try opening objects that cant be opened
    {
        newsflash::bigfile file;

        BOOST_REQUIRE(file.open("."));
        BOOST_REQUIRE(!file.is_open());
#if defined(LINUX_OS)
        BOOST_REQUIRE(file.open("/dev/mem")); // no permission 
//        REQUIRE_EXCEPTION(newsflash::bigfile::size("/dev/mem"));
//        REQUIRE_EXCEPTION(newsflash::bigfile::resize("/dev/mem", 100));
#elif defined(WINDOWS_OS)
        BOOST_REQUIRE(file.open("\\\foobar\file"));
#endif
    }

    // try open the file for appending, file doesn't exist it's created
    {

        delete_file("test0.file");

        newsflash::bigfile file;

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
        newsflash::bigfile file;

        BOOST_REQUIRE(!file.create("test0.file"));
        BOOST_REQUIRE(file.is_open());
        BOOST_REQUIRE(newsflash::bigfile::size("test0.file") == 0);        
        file.close();

        generate_file("test0.file", 1024);

        // open existing file and truncate it's contents
        BOOST_REQUIRE(!file.create("test0.file"));
        BOOST_REQUIRE(newsflash::bigfile::size("test0.file") == 0);

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
        newsflash::bigfile file;

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

        newsflash::bigfile file;
        BOOST_REQUIRE(!file.open("test1.file"));

        BOOST_REQUIRE(file.read(&empty[0], 512) == 512);

        delete_file("test1.file");
    }
}


void test_large_file()
{
    delete_file("test2.file");

    newsflash::bigfile file;
    file.create("test2.file");

    using big_t = newsflash::bigfile::big_t;

    newsflash::bigfile::resize("test2.file", big_t(0xffffffffL) + 1);
    BOOST_REQUIRE(newsflash::bigfile::size("test2.file") == big_t(0xffffffffL) + 1);

    const char buff[] = "foobar";

    file.seek(0);
    file.write(buff, sizeof(buff));

    file.seek(big_t(0xFFFFFFFFL));
    file.write(buff, 1);
    file.close();

    file.append("test2.file");
    file.write(buff, sizeof(buff));

    BOOST_REQUIRE(newsflash::bigfile::size("test2.file") == big_t(0xffffffffL) + sizeof(buff));

    delete_file("test2.file");    
}

int test_main(int, char* [])
{
    test_basic_file_ops();
    test_file_write_read();

    return 0;
}

