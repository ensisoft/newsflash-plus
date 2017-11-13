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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <boost/test/minimal.hpp>
#include "newsflash/warnpop.h"

#include "engine/decode.h"
#include "unit_test_common.h"

namespace nf = newsflash;

void unit_test_yenc_single()
{
    // successful DecodeJob
    {
        nf::DecodeJob dec(read_file_buffer("test_data/newsflash_2_0_0.yenc"));
        dec.perform();

        const auto png = read_file_contents("test_data/newsflash_2_0_0.png");

        BOOST_REQUIRE(dec.GetBinaryOffset() == 0);
        BOOST_REQUIRE(dec.GetBinarySize() == png.size());
        BOOST_REQUIRE(dec.GetBinaryName() == "test.png");
        BOOST_REQUIRE(dec.GetErrors().any_bit() == false);
        BOOST_REQUIRE(dec.GetEncoding() == nf::DecodeJob::Encoding::yEnc);

        const auto& bin = dec.GetBinaryData();
        const auto& txt = dec.GetTextData();
        BOOST_REQUIRE(txt.empty() == true);
        BOOST_REQUIRE(bin.empty() == false);

        BOOST_REQUIRE(bin == png);
    }

    // error (throws an exception)
    {
        nf::DecodeJob dec(read_file_buffer("test_data/newsflash_2_0_0.broken.yenc"));
        dec.perform();
        BOOST_REQUIRE(dec.has_exception());
    }

    // damaged
    {
        nf::DecodeJob dec(read_file_buffer("test_data/newsflash_2_0_0.damaged.yenc"));
        dec.perform();

        const auto err = dec.GetErrors();
        BOOST_REQUIRE(err.test(nf::DecodeJob::Error::CrcMismatch));
        BOOST_REQUIRE(err.test(nf::DecodeJob::Error::SizeMismatch));
    }
}

void unit_test_yenc_multi()
{
    // successful DecodeJob
    {
        const auto jpg = read_file_contents("test_data/1489406.jpg");

        std::vector<char> out;
        out.resize(jpg.size());

        std::size_t offset = 0;
        // first part
        {
            nf::DecodeJob dec(read_file_buffer("test_data/1489406.jpg-001.ync"));
            dec.perform();

            BOOST_REQUIRE(dec.GetBinaryOffset() == offset);
            BOOST_REQUIRE(dec.GetBinarySize() == jpg.size());
            BOOST_REQUIRE(dec.GetBinaryName() == "1489406.jpg");
            BOOST_REQUIRE(dec.GetErrors().any_bit() == false);

            const auto& bin = dec.GetBinaryData();
            std::memcpy(&out[offset], bin.data(), bin.size());
            offset = bin.size();
        }

        // second part
        {
            nf::DecodeJob dec(read_file_buffer("test_data/1489406.jpg-002.ync"));
            dec.perform();

            BOOST_REQUIRE(dec.GetBinaryOffset() == offset);
            BOOST_REQUIRE(dec.GetBinarySize() == jpg.size());
            BOOST_REQUIRE(dec.GetBinaryName() == "1489406.jpg");
            BOOST_REQUIRE(dec.GetErrors().any_bit() == false);

            const auto& bin = dec.GetBinaryData();
            std::memcpy(&out[offset], bin.data(), bin.size());
            offset += bin.size();
        }

        // third part
        {
            nf::DecodeJob dec(read_file_buffer("test_data/1489406.jpg-003.ync"));
            dec.perform();

            BOOST_REQUIRE(dec.GetBinaryOffset() == offset);
            BOOST_REQUIRE(dec.GetBinarySize() == jpg.size());
            BOOST_REQUIRE(dec.GetBinaryName() == "1489406.jpg");
            BOOST_REQUIRE(dec.GetErrors().any_bit() == false);

            const auto& bin = dec.GetBinaryData();
            std::memcpy(&out[offset], bin.data(), bin.size());
            offset += bin.size();
        }
        BOOST_REQUIRE(offset += jpg.size());
        BOOST_REQUIRE(jpg == out);
    }

    // error (throws an exception)
    {
        nf::DecodeJob dec(read_file_buffer("test_data/1489406.jpg-001.broken.ync"));
        dec.perform();
        BOOST_REQUIRE(dec.has_exception());
    }

    // damaged
    {
        nf::DecodeJob dec(read_file_buffer("test_data/1489406.jpg-001.damaged.ync"));
        dec.perform();

        const auto flags = dec.GetErrors();
        BOOST_REQUIRE(flags.any_bit());
    }
}

void unit_test_uuencode_single()
{
    // successful DecodeJob
    {
        nf::DecodeJob dec(read_file_buffer("test_data/newsflash_2_0_0.uuencode"));
        dec.perform();

        const auto png = read_file_contents("test_data/newsflash_2_0_0.png");
        BOOST_REQUIRE(dec.GetBinaryOffset() == 0);
        BOOST_REQUIRE(dec.GetBinarySize() == 0);
        BOOST_REQUIRE(dec.GetBinaryName() == "test");
        BOOST_REQUIRE(dec.IsMultipart() == false);
        BOOST_REQUIRE(dec.IsFirstPart() == true);
        BOOST_REQUIRE(dec.IsLastPart() == true);

        const auto& bin = dec.GetBinaryData();
        BOOST_REQUIRE(bin == png);
    }

    // uuencode doesn't support any crc/size error checking or
    // multipart binaries with offsets.
}

void unit_test_uuencode_multi()
{
    // succesful DecodeJob
    {
        std::vector<char> out;

        // first part
        {
            nf::DecodeJob dec(read_file_buffer("test_data/1489406.jpg-001.uuencode"));
            dec.perform();

            BOOST_REQUIRE(dec.GetBinaryOffset() == 0);
            BOOST_REQUIRE(dec.GetBinarySize() == 0);
            BOOST_REQUIRE(dec.GetBinaryName() == "1489406.jpg");
            BOOST_REQUIRE(dec.IsMultipart() == true);
            BOOST_REQUIRE(dec.IsFirstPart() == true);
            BOOST_REQUIRE(dec.IsLastPart() == false);

            const auto& bin = dec.GetBinaryData();
            std::copy(bin.begin(), bin.end(), std::back_inserter(out));

        }

        // second part
        {
            nf::DecodeJob dec(read_file_buffer("test_data/1489406.jpg-002.uuencode"));
            dec.perform();

            BOOST_REQUIRE(dec.GetBinaryOffset() == 0);
            BOOST_REQUIRE(dec.GetBinarySize() == 0);
            BOOST_REQUIRE(dec.GetBinaryName() == "");
            BOOST_REQUIRE(dec.IsMultipart() == true);
            BOOST_REQUIRE(dec.IsFirstPart() == false);
            BOOST_REQUIRE(dec.IsLastPart() == false);

            const auto& bin = dec.GetBinaryData();
            std::copy(bin.begin(), bin.end(), std::back_inserter(out));

        }

        // third part
        {
            nf::DecodeJob dec(read_file_buffer("test_data/1489406.jpg-003.uuencode"));
            dec.perform();

            BOOST_REQUIRE(dec.GetBinaryOffset() == 0);
            BOOST_REQUIRE(dec.GetBinarySize() == 0);
            BOOST_REQUIRE(dec.GetBinaryName() == "");
            BOOST_REQUIRE(dec.IsMultipart() == true);
            BOOST_REQUIRE(dec.IsFirstPart() == false);
            BOOST_REQUIRE(dec.IsLastPart() == true);

            const auto& bin = dec.GetBinaryData();
            std::copy(bin.begin(), bin.end(), std::back_inserter(out));
        }

        const auto jpg = read_file_contents("test_data/1489406.jpg");

        BOOST_REQUIRE(jpg == out);
    }
}

int test_main(int, char*[])
{
    unit_test_yenc_single();
    unit_test_yenc_multi();
    unit_test_uuencode_single();
    unit_test_uuencode_multi();

    return 0;
}