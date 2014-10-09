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

// $Id: unit_test_common.h,v 1.5 2009/09/04 19:10:21 svaisane Exp $

#pragma once

#include <newsflash/config.h>

#ifdef TEST_CUSTOM
#  define BOOST_REQUIRE TEST_REQUIRE
#endif

//#include <boost/test/minimal.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <newsflash/config.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <vector>
#include <ctime>

#include "../buffer.h"

  //#define BOOST_FILESYSTEM_VERSION 2

#if defined(WINDOWS_OS)
#  include <windows.h>
#elif defined(LINUX_OS)
#  include <unistd.h>
#endif

newsflash::buffer read_file_buffer(const char* file)
{
    auto* f = std::fopen(file, "rb");
    BOOST_REQUIRE(f);

    std::fseek(f, 0, SEEK_END);
    const auto size = ftell(f);
    std::fseek(f, 0, SEEK_SET);

    newsflash::buffer buff(size);

    std::fread(buff.back(), 1, size, f);
    std::fclose(f);

    buff.append(size);
    buff.set_content_type(newsflash::buffer::type::article);
    buff.set_content_length(size);
    buff.set_content_start(0);

    return std::move(buff);
}



int random_int()
{
    struct randomizer {
        randomizer() {
            std::srand(std::time(nullptr));
        }
    };
    static randomizer r;

    return rand();
}


inline
std::vector<char> read_file_contents(const char* file)
{
    FILE* p = fopen(file, "rb");
    
    fseek(p, 0, SEEK_END);
    long size = ftell(p);
    fseek(p, 0, SEEK_SET);

    std::vector<char> buff;    
    buff.resize(size);

    fread(&buff[0], 1, size, p);
    fclose(p);

    return buff;
}

inline
void fill_random(char* buff, size_t bytes)
{
    for (size_t i=0; i<bytes; ++i)
        buff[i] = ((bytes * i ) ^ random_int());
}

inline
int strcasecmp(const char* first, const char* second, size_t n)
{
    int i = 0;
#if defined(WINDOWS_OS)
    i = _strnicmp(first, second, n);
#elif defined(LINUX_OS)
    i = ::strncasecmp(first, second, n);
#endif
    if (i < 0)
        return -1;
    if (i > 0)
        return 1;
    return i;
}

inline
int strcasecmp(const char* first, const char* second)
{
    int i = 0;
#if defined(WINDOWS_OS)
    i = _stricmp(first, second);
#elif defined(LINUX_OS)
    i = strcasecmp(first, second);
#endif 
    if (i < 0)
        return -1;
    if (i > 0)
        return 1;
    return i;
}

inline
void delete_file(const char* file)
{
    try
    {
        boost::filesystem::path ph(file);
        boost::filesystem::remove(ph);
    }
    catch (...) 
    {
        std::cerr << "(DONT PANIC!) failed to delete file: " << file << std::endl;
    }
}

inline 
void sleepms(int millis)
{
#if defined(WINDOWS_OS)
    Sleep(millis);
#elif defined(LINUX_OS)
    usleep(millis*1000);
#endif

}

inline
bool file_exists(const char* file)
{
    std::ifstream in(file);
    return in.is_open();
}

inline
std::string illegal_fs_root()
{
#if defined(WINDOWS)
    return "ö:\\";
#elif defined(LINUX_OS)
    return "/root/";
#endif

}

inline
void write_file(const char* file, const char* data)
{
    std::ofstream out(file, std::ios::out | std::ios::trunc);
    BOOST_REQUIRE(out.is_open());
    out << data;
}

inline
void generate_file(const char* file, size_t size)
{
    std::vector<char> buff;
    buff.resize(size);

    fill_random(&buff[0], size);

    std::ofstream out(file, std::ios::out | std::ios::binary | std::ios::trunc);
    BOOST_REQUIRE(out.is_open());

    out.write(&buff[0], size);
}

inline
void append_file(const char* file, const char* data)
{
    std::ofstream out(file, std::ios::out | std::ios::app);
    out << data;
}

inline
void create_dir(const char* name)
{
    try
    {
        boost::filesystem::path ph(name);
        boost::filesystem::create_directory(ph);
    }
    catch (...) {}
}


#define REQUIRE_EXCEPTION(expr) \
    do { \
        try { \
            expr; \
            BOOST_REQUIRE(!"exception was expected!"); \
        } catch (const std::exception&) {} \
    } while (0)



