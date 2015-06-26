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

#include <boost/test/minimal.hpp>
#include "../uuencode_decoder.h"
#include "../uuencode.h"
#include "unit_test_common.h"

struct tester 
{
    std::vector<char> binary;
    std::vector<newsflash::decoder::error> errors;
    newsflash::decoder::info info;

    tester(newsflash::decoder& dec)
    {
        dec.on_write = std::bind(&tester::decoder_write, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
        dec.on_info = std::bind(&tester::decoder_info, this,
            std::placeholders::_1);
        dec.on_error = std::bind(&tester::decoder_error, this,
            std::placeholders::_1, std::placeholders::_2);
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
    void decoder_error(const newsflash::decoder::error error, const std::string& str)
    {
        errors.push_back(error);
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

    std::vector<char> uu;
    uuencode::encode(data.begin() + cur_size, data.end(), std::back_inserter(uu));    
    return uu;
}

void test_single_success()
{
    const std::string header("begin 655 foo.png\r\n");
    const std::string footer("`\r\nend");

    std::vector<char> part;
    std::vector<char> data;

    append(part, header);
    append(part, encode(data, 1024));
    append(part, footer);

    newsflash::uuencode_decoder dec;
    tester test(dec);

    dec.decode(part.data(), part.size());
    BOOST_REQUIRE(test.info.name == "foo.png");
    BOOST_REQUIRE(test.info.size == 0);
    BOOST_REQUIRE(test.binary == data);
}

void test_single_broken()
{
    // todo:
}

int test_main(int, char*[])
{
    test_single_success();
    test_single_broken();

    return 0;
}