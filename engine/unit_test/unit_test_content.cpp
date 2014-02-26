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
#include <cstdlib>
#include "../content.h"
#include "../decoder.h"
#include "../buffer.h"
#include "unit_test_common.h"

class test_decoder : public newsflash::decoder
{
public:
    test_decoder() : offset_(0)
    {}

    virtual void decode(const char* data, std::size_t len) override
    {
        on_write(data, len, offset_);
        offset_ += len;
    }

    virtual void finish() override
    {}

    virtual void cancel() override
    {}
private:
    std::size_t offset_;
};

std::shared_ptr<newsflash::buffer> generate_buffer(std::size_t id, std::size_t bytes)
{
    auto buff = std::make_shared<newsflash::buffer>(id, bytes);

    for (size_t i=0; i<bytes; ++i)
        (*buff)[i] = 0xff & (random_int() >> 7);

    buff->resize(bytes);
    return buff;
}

std::shared_ptr<newsflash::buffer> read_buffer_from_file(const char* file, std::size_t offset, std::size_t len)
{
    std::FILE* fp = fopen(file, "rb");
    BOOST_REQUIRE(fp);

    fseek(fp, SEEK_SET, offset);

    auto buff = std::make_shared<newsflash::buffer>(0, len);

    fread(&(*buff)[0], 1, len, fp);
    fclose(fp);

    buff->resize(len);
    return buff;
}

void test_success()
{
    // test single buffer
    {
        delete_file("test.png");

        const auto buff = generate_buffer(0, 512);

        newsflash::content test("", "test.png", std::unique_ptr<test_decoder>(new test_decoder));
        test.decode(buff);
        test.finish();
        BOOST_REQUIRE(test.good());

        const auto file = read_buffer_from_file("test.png", 0, 512);

        BOOST_REQUIRE(*file == *buff);

        delete_file("test.png");
    }

    // test multiple buffers
    {
        delete_file("test.png");

        const auto part1 = generate_buffer(0, 512);
        const auto part2 = generate_buffer(1, 100);

        newsflash::content test(".", "test.png", std::unique_ptr<test_decoder>(new test_decoder));
        test.decode(part1);
        test.decode(part2);
        test.finish();
        BOOST_REQUIRE(test.good());

        const auto file1 = read_buffer_from_file("test.png", 0, 512);
        const auto file2 = read_buffer_from_file("test.png", 512, 100);
        BOOST_REQUIRE(*file1 == *part1);
        BOOST_REQUIRE(*file2 == *part2);

        delete_file("test.png");
    }

}


void test_decode_error()
{

}

void test_filesystem_error()
{

}

int test_main(int, char* [])
{
    test_success();

    return 0;
}

