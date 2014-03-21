// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#include <boost/test/minimal.hpp>
#include "../download.h"
#include "unit_test_common.h"

corelib::buffer generate_buffer(std::size_t id, std::size_t bytes)
{
    corelib::buffer buff;
    buff.resize(bytes);

    for (size_t i=0; i<bytes; ++i)
        buff[i] = 0xff & (random_int() >> 7);

    return buff;
}

corelib::buffer read_file(const char* file, std::size_t offset, std::size_t len)
{
    std::FILE* fp = fopen(file, "rb");
    BOOST_REQUIRE(fp);

    fseek(fp, offset, SEEK_SET);

    corelib::buffer buff;
    buff.resize(len);

    fread(&buff[0], 1, len, fp);
    fclose(fp);

    return buff;
}

void test_yenc_single()
{
    delete_file("test.png");

    

    delete_file("test.png");
}

void test_yenc_multi()
{

}

void test_unknown()
{

}

void test_uuencode()
{

}

void test_uuencode_with_text()
{

}

void test_non_overwrite()
{

}

void test_decoding_error()
{

}

void test_cancel()
{

}

int test_main(int, char*[])
{
    return 0;
}