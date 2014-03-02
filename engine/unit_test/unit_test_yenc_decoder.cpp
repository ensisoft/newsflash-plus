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
#include "unit_test_common.h"

struct tester 
{
    std::vector<char> binary;
    std::vector<std::string> errors;
    newsflash::decoder::info info;

    tester(newsflash::decoder& dec)
    {
        dec.on_write = std::bind(&tester::decoder_write, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        dec.on_info = std::bind(&tester::decoder_info, this,
            std::placeholders::_1);
        dec.on_error = std::bind(&tester::decoder_error, this,
            std::placeholders::_1);
    }

    void decoder_write(const void* data, std::size_t size, std::size_t offset)
    {
        if (binary.size() < size + offset)
            binary.resize(size + offset);
        std::memcpy(&binary[offset], data, size);
    }

    void decoder_info(const newsflash::decoder::info& info)
    {
        this->info = info;
    }
    void decoder_error(const std::string& err)
    {
        errors.push_back(err);
    }
};

void test_single_success()
{
    newsflash::yenc_single_decoder yenc;

    tester test(yenc);
}

void test_single_broken()
{

}

void test_single_error()
{

}

int test_main(int, char*[])
{
    test_single_success();
    test_single_broken();
    test_single_error();

    return 0;
}