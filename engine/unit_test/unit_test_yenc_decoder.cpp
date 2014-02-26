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
#include "../yenc_single_decoder.h"
#include "../yenc_multi_decoder.h"

void decoder_write(const void* data, std::size_t size, std::size_t offset)
{}

void decoder_info(const newsflash::decoder::info& info)
{}

void decoder_error(const std::string& err)
{}


void test_single_success()
{
    newsflash::yenc_single_decoder yenc;
    yenc.on_write = decoder_write;
    yenc.on_info  = decoder_info;

    //yenc.decode()
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