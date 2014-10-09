// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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

#include <newsflash/config.h>

#include <boost/test/minimal.hpp>
#include "../decode.h"
#include "unit_test_common.h"

namespace nf = newsflash;

void unit_test_yenc_single()
{
    // successful decode
    {
        nf::decode dec(read_file_buffer("test_data/newsflash_2_0_0.yenc"));
        dec.perform();

        const auto png = read_file_contents("test_data/newsflash_2_0_0.png");

        BOOST_REQUIRE(dec.get_binary_offset() == 0);
        BOOST_REQUIRE(dec.get_binary_size() == png.size());
        BOOST_REQUIRE(dec.get_binary_name() == "test.png");
        BOOST_REQUIRE(dec.get_errors().any_bit() == false);
        BOOST_REQUIRE(dec.get_encoding() == nf::encoding::yenc_single);

        const auto& bin = dec.get_binary_data();
        const auto& txt = dec.get_text_data();
        BOOST_REQUIRE(txt.empty() == true);
        BOOST_REQUIRE(bin.empty() == false);

        BOOST_REQUIRE(bin == png);
    }

    // error (throws an exception)
    {
        nf::decode dec(read_file_buffer("test_data/newsflash_2_0_0.broken.yenc"));
        dec.perform();
        BOOST_REQUIRE(dec.has_exception());
    }

    // damaged
    {
        nf::decode dec(read_file_buffer("test_data/newsflash_2_0_0.damaged.yenc"));
        dec.perform();

        const auto err = dec.get_errors();
        BOOST_REQUIRE(err.test(nf::decode::error::crc_mismatch));
        BOOST_REQUIRE(err.test(nf::decode::error::size_mismatch));
    }
}

void unit_test_yenc_multi()
{
    // successful decode
    {
        const auto jpg = read_file_contents("test_data/1489406.jpg");

        std::vector<char> out;
        out.resize(jpg.size());

        std::size_t offset = 0;
        // first part
        {
            nf::decode dec(read_file_buffer("test_data/1489406.jpg-001.ync"));
            dec.perform();

            BOOST_REQUIRE(dec.get_binary_offset() == offset);
            BOOST_REQUIRE(dec.get_binary_size() == jpg.size());
            BOOST_REQUIRE(dec.get_binary_name() == "1489406.jpg");
            BOOST_REQUIRE(dec.get_errors().any_bit() == false);

            const auto& bin = dec.get_binary_data();
            std::memcpy(&out[offset], bin.data(), bin.size());
            offset = bin.size();
        }

        // second part
        {
            nf::decode dec(read_file_buffer("test_data/1489406.jpg-002.ync"));
            dec.perform();

            BOOST_REQUIRE(dec.get_binary_offset() == offset);
            BOOST_REQUIRE(dec.get_binary_size() == jpg.size());
            BOOST_REQUIRE(dec.get_binary_name() == "1489406.jpg");
            BOOST_REQUIRE(dec.get_errors().any_bit() == false);

            const auto& bin = dec.get_binary_data();
            std::memcpy(&out[offset], bin.data(), bin.size());
            offset += bin.size();
        }

        // third part
        {
            nf::decode dec(read_file_buffer("test_data/1489406.jpg-003.ync"));
            dec.perform();

            BOOST_REQUIRE(dec.get_binary_offset() == offset);
            BOOST_REQUIRE(dec.get_binary_size() == jpg.size());
            BOOST_REQUIRE(dec.get_binary_name() == "1489406.jpg");
            BOOST_REQUIRE(dec.get_errors().any_bit() == false);

            const auto& bin = dec.get_binary_data();
            std::memcpy(&out[offset], bin.data(), bin.size());
            offset += bin.size();
        }
        BOOST_REQUIRE(offset += jpg.size());
        BOOST_REQUIRE(jpg == out);
    } 

    // error (throws an exception)
    {
        nf::decode dec(read_file_buffer("test_data/1489406.jpg-001.broken.ync"));
        dec.perform();
        BOOST_REQUIRE(dec.has_exception());
    }

    // damaged
    {
        nf::decode dec(read_file_buffer("test_data/1489406.jpg-001.damaged.ync"));
        dec.perform();

        const auto flags = dec.get_errors();
        BOOST_REQUIRE(flags.any_bit());
    }
}

void unit_test_uuencode_single()
{
    // successful decode
    {
        nf::decode dec(read_file_buffer("test_data/newsflash_2_0_0.uuencode"));
        dec.perform();

        const auto png = read_file_contents("test_data/newsflash_2_0_0.png");
        BOOST_REQUIRE(dec.get_binary_offset() == 0);
        BOOST_REQUIRE(dec.get_binary_size() == 0);
        BOOST_REQUIRE(dec.get_binary_name() == "test");

        const auto& bin = dec.get_binary_data();
        BOOST_REQUIRE(bin == png);
    }

    // uuencode doesn't support any crc/size error checking or 
    // multipart binaries with offsets.
}

void unit_test_uuencode_multi()
{
    // todo:    
}

int test_main(int, char*[])
{
    unit_test_yenc_single();
    unit_test_yenc_multi();

    unit_test_uuencode_single();
    unit_test_uuencode_multi();

    return 0;
}