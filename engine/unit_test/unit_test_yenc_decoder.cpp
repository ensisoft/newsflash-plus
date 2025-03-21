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
#include <boost/crc.hpp>
#include <vector>
#include <cstring>
#include "../decode.h"
#include "../yenc.h"
#include "unit_test_common.h"

template<typename Container>
void append(newsflash::buffer& buff, const Container& data)
{
    if (buff.available() < data.size())
    {
        const auto more = data.size() - buff.available();
        buff.allocate(more);
    }
    std::memcpy(buff.back(), data.data(), data.size());
}

std::vector<char> encode(const std::vector<char>& data)
{
    std::vector<char> yenc;
    yenc::encode(data.begin(), data.end(), std::back_inserter(yenc), 128, true);
    return yenc;
}


void test_single_success()
{
    // generate a single part yenc binary
    const std::string header("=ybegin line=128 size=1024 name=test-data.png\r\n");
    const std::string footer("=yend size=1024\r\n");

    const auto binary = generate_buffer(1024);
    const auto yenc   = encode(binary);

    newsflash::buffer buff(2048);
    append(buff, header);
    append(buff, yenc);
    append(buff, footer);

    newsflash::decode dec(std::move(buff));
    dec.perform();

    //BOOST_REQUIRE(test.info.name == "test-data.png");
    //BOOST_REQUIRE(test.info.size == 1024);
    //BOOST_REQUIRE(test.binary == data);

}

// // decoding completes, but there are problems. binary may be corrupt.
// void test_single_broken()
// {
//     // incorrect crc
//     {
//         const std::string header("=ybegin line=128 size=5666 name=foo.keke\r\n");
//         const std::string footer("=yend size=5666 crc32=dfdfdfdf");

//         std::vector<char> part;
//         std::vector<char> data;

//         append(part, header);
//         append(part, encode(data, 5666));
//         append(part, footer);

//         corelib::yenc_single_decoder yenc;
//         tester test(yenc);
//         yenc.decode(part.data(), part.size());

//         BOOST_REQUIRE(test.info.name == "foo.keke");
//         BOOST_REQUIRE(test.info.size == 5666);
//         BOOST_REQUIRE(test.binary == data);

//         BOOST_REQUIRE(test.errors.size() == 1);
//         BOOST_REQUIRE(test.errors[0] == corelib::decoder::error::crc);
//     }

//     // problems with yenc sizes
//     {

//         const std::string header("=ybegin line=128 size=100 name=testtest\r\n");
//         const std::string footer("=yend size=500");

//         std::vector<char> data;        
//         std::vector<char> part;

//         append(part, header);
//         append(part, encode(data, 4000));
//         append(part, footer);

//         corelib::yenc_single_decoder yenc;
//         tester test(yenc);
//         yenc.decode(part.data(), part.size());

//         BOOST_REQUIRE(test.info.name == "testtest");
//         BOOST_REQUIRE(test.binary == data);
//         BOOST_REQUIRE(test.errors.size() == 2);
//         BOOST_REQUIRE(test.errors[0] == corelib::decoder::error::size);
//         BOOST_REQUIRE(test.errors[1] == corelib::decoder::error::size);
//     }
// }

// // broken yenc data (decoding cannot continue)
// void test_single_error()
// {
//     // broken header
//     {
//         const std::string header("=ybegin foobar sizde=100\r\n");
//         const std::string footer("=yend size=1024");

//         std::vector<char> part;
//         std::vector<char> data;

//         append(part, header);
//         append(part, encode(data, 4000));
//         append(part, footer);

//         corelib::yenc_single_decoder yenc;

//         REQUIRE_EXCEPTION(yenc.decode(part.data(), part.size()));
//     }

//     // missing footer
//     {
//         const std::string header("=ybegin size=1024 line=128 name=pelle-peloton\r\n");

//         std::vector<char> part;
//         std::vector<char> data;

//         append(part, header);
//         append(part, encode(data, 4000));

//         corelib::yenc_single_decoder yenc;
//         REQUIRE_EXCEPTION(yenc.decode(part.data(), part.size()));
//     }
// }

// void test_multi_success()
// {
//     std::vector<char> data1;
//     std::vector<char> data2;

//     corelib::yenc_multi_decoder yenc;
//     tester test(yenc);

//     std::vector<char> data;

//     // first part
//     { 
//         const std::string header("=ybegin part=1 total=2 line=128 size=2048 name=test\r\n");
//         const std::string part("=ypart begin=1 end=1024\r\n");
//         const std::string footer("=yend size=1024 part=1\r\n");

//         std::vector<char> yenc_data;
//         append(yenc_data, header);
//         append(yenc_data, part);
//         append(yenc_data, encode(data, 1024));
//         append(yenc_data, footer);
//         yenc.decode(yenc_data.data(), yenc_data.size());

//     }

//     // second part
//     {
//         const std::string header("=ybegin part=2 total=2 line=128 size=2048 name=test\r\n");
//         const std::string part("=ypart begin=1025 end=2048\r\n");    
//         const std::string footer("=yend size=1024 part=2\r\n");   

//         std::vector<char> yenc_data;
//         append(yenc_data, header);
//         append(yenc_data, part);
//         append(yenc_data, encode(data, 1024));
//         append(yenc_data, footer);
//         yenc.decode(yenc_data.data(), yenc_data.size());
//     }

//     yenc.finish();

//     BOOST_REQUIRE(test.info.name == "test");
//     BOOST_REQUIRE(test.info.size == 2048);
//     BOOST_REQUIRE(test.binary == data);

// }


// void test_multi_broken()
// {
//     // crc error
//     {
//         // todo:
//     }

// }

// void test_multi_error()
// {
//     // broken header
//     {
//         // todo:
//     }
// }

int test_main(int, char*[])
{
    // test_single_success();
    // test_single_broken();
    // test_single_error();

    // test_multi_success();
    // test_multi_broken();
    // test_multi_error();

    return 0;
}