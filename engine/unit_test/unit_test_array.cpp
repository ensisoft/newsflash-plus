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

#include <newsflash/config.h>
#include <boost/test/minimal.hpp>
#include "../array.h"
#include "../filebuf.h"
#include "../filemap.h"
#include "unit_test_common.h"


int test_main(int, char*[])
{
    namespace n = newsflash;

    delete_file("array.dat");

    {
        using array = n::array<int, n::filebuf>;

        array arr;
        arr.open("array.dat");
        arr.resize(100);

        arr[0]  = 123;
        arr[99] = 99;

        BOOST_REQUIRE(arr[0] == 123);
        BOOST_REQUIRE(arr[99] == 99);

        arr.resize(150);
        BOOST_REQUIRE(arr[0] == 123);
        BOOST_REQUIRE(arr[99] == 99);

        arr[149] = 149;
        BOOST_REQUIRE(arr[149] == 149);        

    }

    {
        using array = n::array<int, n::filebuf>;

        array arr;
        arr.open("array.dat");
        BOOST_REQUIRE(arr.size() == 150);
        BOOST_REQUIRE(arr[0] == 123);
        BOOST_REQUIRE(arr[99] == 99);
        BOOST_REQUIRE(arr[149] == 149);
    }

    return 0;
}