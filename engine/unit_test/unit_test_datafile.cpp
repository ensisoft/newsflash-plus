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
#include <newsflash/warnpush.h>
#  include <boost/test/minimal.hpp>
#include <newsflash/warnpop.h>
#include <thread>
#include "unit_test_common.h"
#include "../datafile.h"
#include "../threadpool.h"

namespace nf = newsflash;

void unit_test_overwrite()
{
    delete_file("datafile");
    delete_file("(2) datafile");

    // rename file to resolve collision when overwrite is false
    {
        nf::datafile file("", "datafile", 0, true, false);
        BOOST_REQUIRE(file.name() == "datafile");
        BOOST_REQUIRE(file.path() == "");
        BOOST_REQUIRE(file_exists("datafile"));

        nf::datafile second("", "datafile", 0, true, false);
        BOOST_REQUIRE(second.name() == "(2) datafile");
        BOOST_REQUIRE(second.path() == "");
        BOOST_REQUIRE(file_exists("(2) datafile"));
    }

    // overwrite existing file
    {
        nf::datafile file("", "datafile", 0, true, true);
        BOOST_REQUIRE(file.name() == "datafile");
        BOOST_REQUIRE(file_exists("datafile"));
    }


    delete_file("datafile");
    delete_file("(2) datafile");    
}

void unit_test_append()
{
    delete_file("datafile");

    // open first file
    {
//        newsflash::datafile file("", "datafile", 0, false, false, false);
//        BOOST_REQUIRE(file.name() == "datafile");
    }

    // open again for append
    {
//        newsflash::datafile file("", "datafile", 0, false, false, true); 
//        BOOST_REQUIRE(file.name() == "datafile");
    }

    delete_file("datafile");
}

void unit_test_discard()
{
    delete_file("datafile"); 

    // single threaded discard
    {
        {
            nf::datafile file("", "datafile", 0, true, true);
            BOOST_REQUIRE(file_exists("datafile"));
            file.discard_on_close();
        }
        BOOST_REQUIRE(!file_exists("datafile"));        
    }

    // discard while pending actions
    {
        // prepare some write actions 
        nf::threadpool threads(4);

        threads.on_complete = [&](nf::action* a) {
            BOOST_REQUIRE(a->has_exception() == false);
            delete a;
        };

        for (int i=0; i<1000; ++i)
        {
            auto file = std::make_shared<nf::datafile>("", "datafile", 0, true, true);

            std::vector<char> buff;
            buff.resize(1024);
            fill_random(&buff[0], buff.size());

            threads.submit(new nf::datafile::write(0, buff, file));
            threads.submit(new nf::datafile::write(0, buff, file));
            threads.submit(new nf::datafile::write(0, buff, file));
            file->discard_on_close();        
            threads.submit(new nf::datafile::write(0, buff, file));
            threads.submit(new nf::datafile::write(0, buff, file));        
            file.reset();                                

            threads.wait_all_actions();            
            BOOST_REQUIRE(!file_exists("datafile"));            
        }

        threads.shutdown();
    }
}


int test_main(int, char*[])
{
    unit_test_overwrite();
    unit_test_append();
    unit_test_discard();

    return 0;
}