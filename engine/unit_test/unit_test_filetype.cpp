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
#include "../filetype.h"

int test_main(int, char*[])
{
    using t = newsflash::filetype;
    namespace n = newsflash;

    BOOST_REQUIRE(n::find_filetype("agajlasg.asd") == t::other);
    BOOST_REQUIRE(n::find_filetype("foobar.png") == t::image);
    BOOST_REQUIRE(n::find_filetype("foobar.PNG") == t::image);
    BOOST_REQUIRE(n::find_filetype("foobar.mp4") == t::video);    
    BOOST_REQUIRE(n::find_filetype("foobar.r00") == t::archive);
    BOOST_REQUIRE(n::find_filetype("foobar.r59") == t::archive);    
    BOOST_REQUIRE(n::find_filetype("foobar.rar") == t::archive);        
    BOOST_REQUIRE(n::find_filetype("foobar.mpeg") == t::video);
    BOOST_REQUIRE(n::find_filetype("foobar.part001.rar") == t::archive);
    BOOST_REQUIRE(n::find_filetype("Battlefield_Bad_Company_2_NTSC_XBOX360-CCCLX_packed.exe") == t::other);



    return 0;
}