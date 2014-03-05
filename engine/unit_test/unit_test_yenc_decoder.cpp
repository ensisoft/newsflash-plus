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
#include <boost/crc.hpp>
#include <vector>
#include <cstring>
#include "../yenc_single_decoder.h"
#include "../yenc_multi_decoder.h"
#include "../yenc.h"
#include "unit_test_common.h"

struct tester 
{
    std::vector<char> binary;
    std::vector<newsflash::decoder::problem> problems;
    newsflash::decoder::info info;

    tester(newsflash::decoder& dec)
    {
        dec.on_write = std::bind(&tester::decoder_write, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
        dec.on_info = std::bind(&tester::decoder_info, this,
            std::placeholders::_1);
        dec.on_problem = std::bind(&tester::decoder_problem, this,
            std::placeholders::_1);
    }

    void decoder_write(const void* data, std::size_t size, std::size_t offset, bool has_offset)
    {
        if (has_offset)
        {
            if (binary.size() < size + offset)
                binary.resize(size + offset);            
            std::memcpy(&binary[offset], data, size);
        }
        else
        {
            const auto start = binary.size();
            binary.resize(binary.size() + size);
            std::memcpy(&binary[start], data, size);
        }
    }

    void decoder_info(const newsflash::decoder::info& info)
    {
        this->info = info;
    }
    void decoder_problem(const newsflash::decoder::problem& problem)
    {
        problems.push_back(problem);
    }
};

template<typename Container, typename Values>
void append(Container& c, const Values& v)
{
    std::copy(v.begin(), v.end(), std::back_inserter(c));
}

std::vector<char> encode(std::vector<char> & data, std::size_t size)
{
    const auto cur_size = data.size();
    data.resize(cur_size + size);
    fill_random(&data[cur_size], size);

    std::vector<char> yenc;
    yenc::encode(data.begin() + cur_size, data.end(), std::back_inserter(yenc), 128, true);
    return yenc;
}


void test_single_success()
{
    // generate a single part yenc binary
    const std::string header("=ybegin line=128 size=1024 name=test-data.png\r\n");
    const std::string footer("=yend size=1024\r\n");

    std::vector<char> part;
    std::vector<char> data;

    append(part, header);
    append(part, encode(data, 1024)); 
    append(part, footer);

    newsflash::yenc_single_decoder yenc;
    tester test(yenc);
    yenc.decode(part);

    BOOST_REQUIRE(test.info.name == "test-data.png");
    BOOST_REQUIRE(test.info.size == 1024);
    BOOST_REQUIRE(test.binary == data);

}

// decoding completes, but there are problems. binary may be corrupt.
void test_single_broken()
{
    // incorrect crc
    {
        const std::string header("=ybegin line=128 size=5666 name=foo.keke\r\n");
        const std::string footer("=yend size=5666 crc32=dfdfdfdf");

        std::vector<char> part;
        std::vector<char> data;

        append(part, header);
        append(part, encode(data, 5666));
        append(part, footer);

        newsflash::yenc_single_decoder yenc;
        tester test(yenc);
        yenc.decode(part);

        BOOST_REQUIRE(test.info.name == "foo.keke");
        BOOST_REQUIRE(test.info.size == 5666);
        BOOST_REQUIRE(test.binary == data);

        BOOST_REQUIRE(test.problems.size() == 1);
        BOOST_REQUIRE(test.problems[0].kind == newsflash::decoder::problem::type::crc);
    }

    // problems with yenc sizes
    {

        const std::string header("=ybegin line=128 size=100 name=testtest\r\n");
        const std::string footer("=yend size=500");

        std::vector<char> data;        
        std::vector<char> part;

        append(part, header);
        append(part, encode(data, 4000));
        append(part, footer);

        newsflash::yenc_single_decoder yenc;
        tester test(yenc);
        yenc.decode(part);

        BOOST_REQUIRE(test.info.name == "testtest");
        BOOST_REQUIRE(test.binary == data);
        BOOST_REQUIRE(test.problems.size() == 2);
        BOOST_REQUIRE(test.problems[0].kind == newsflash::decoder::problem::type::size);
        BOOST_REQUIRE(test.problems[1].kind == newsflash::decoder::problem::type::size);
    }
}

// broken yenc data (decoding cannot continue)
void test_single_error()
{
    // broken header
    {
        const std::string header("=ybegin foobar sizde=100\r\n");
        const std::string footer("=yend size=1024");

        std::vector<char> part;
        std::vector<char> data;

        append(part, header);
        append(part, encode(data, 4000));
        append(part, footer);

        newsflash::yenc_single_decoder yenc;

        REQUIRE_EXCEPTION(yenc.decode(part));
    }

    // missing footer
    {
        const std::string header("=ybegin size=1024 line=128 name=pelle-peloton\r\n");

        std::vector<char> part;
        std::vector<char> data;

        append(part, header);
        append(part, encode(data, 4000));

        newsflash::yenc_single_decoder yenc;
        REQUIRE_EXCEPTION(yenc.decode(part));
    }
}

void test_multi_success()
{
    std::vector<char> data1;
    std::vector<char> data2;

    newsflash::yenc_multi_decoder yenc;
    tester test(yenc);

    std::vector<char> data;

    // first part
    { 
        const std::string header("=ybegin part=1 total=2 line=128 size=2048 name=test\r\n");
        const std::string part("=ypart begin=1 end=1024\r\n");
        const std::string footer("=yend size=1024 part=1\r\n");

        std::vector<char> yenc_data;
        append(yenc_data, header);
        append(yenc_data, part);
        append(yenc_data, encode(data, 1024));
        append(yenc_data, footer);
        yenc.decode(yenc_data);

    }

    // second part
    {
        const std::string header("=ybegin part=2 total=2 line=128 size=2048 name=test\r\n");
        const std::string part("=ypart begin=1025 end=2048\r\n");    
        const std::string footer("=yend size=1024 part=2\r\n");   

        std::vector<char> yenc_data;
        append(yenc_data, header);
        append(yenc_data, part);
        append(yenc_data, encode(data, 1024));
        append(yenc_data, footer);
        yenc.decode(yenc_data);
    }

    yenc.finish();

    BOOST_REQUIRE(test.info.name == "test");
    BOOST_REQUIRE(test.info.size == 2048);
    BOOST_REQUIRE(test.binary == data);

}


void test_multi_broken()
{}

void test_multi_error()
{}

int test_main(int, char*[])
{
    test_single_success();
    test_single_broken();
    test_single_error();

    test_multi_success();
    test_multi_broken();
    test_multi_error();

    return 0;
}