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

#include <cstring>

#include "engine/buffer.h"

int test_main(int, char*[])
{
    newsflash::buffer buff(1024);
    BOOST_REQUIRE(buff.size() == 0);
    BOOST_REQUIRE(buff.available() == 1024);

    const char* str = "jeesus ajaa mopolla";

    std::strcpy(buff.back(), str);
    buff.append(std::strlen(str));

    BOOST_REQUIRE(buff.size() == std::strlen(str));

    auto other = buff.split(6);

    BOOST_REQUIRE(other.size() == 6);
    BOOST_REQUIRE(buff.size() == std::strlen(str) - 6);
    BOOST_REQUIRE(!std::memcmp(buff.head(), &str[6], buff.size()));
    BOOST_REQUIRE(!std::memcmp(other.head(), "jeesus", 6));

    return 0;
}