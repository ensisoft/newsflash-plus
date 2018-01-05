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

#include "app/debug.h"

int test_main(int, char* argv[])
{
    namespace d = debug;

    BOOST_REQUIRE(d::toString("foobar") == "foobar");
    BOOST_REQUIRE(d::toString("hello %1", "world") == "hello world");
    BOOST_REQUIRE(d::toString("value %1", 123) == "value 123");
    BOOST_REQUIRE(d::toString("value %1 and value %2", 333, 444) == "value 333 and value 444");

    {
        DEBUG("hello %1 world", "beautiful");
        DEBUG("my int is %1 my double is %2", 123, 56.0);
    }

    {
        DEBUG("Unrar error %1", QProcess::WriteError);
    }

    for (int i=0; i<100; ++i)
    {
        DEBUG("ping");
    }

    return 0;
}