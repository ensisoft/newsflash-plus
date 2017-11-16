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
#include <thread>

#include "engine/datafile.h"
#include "engine/threadpool.h"
#include "unit_test_common.h"


namespace nf = newsflash;

void unit_test_overwrite()
{
    delete_file("DataFile");
    delete_file("(2) DataFile");

    // rename file to resolve collision when overwrite is false
    {
        nf::DataFile file("", "DataFile", 0, true, false);
        BOOST_REQUIRE(file.GetFileName() == "DataFile");
        BOOST_REQUIRE(file.GetFilePath() == "");
        BOOST_REQUIRE(file_exists("DataFile"));

        nf::DataFile second("", "DataFile", 0, true, false);
        BOOST_REQUIRE(second.GetFileName() == "(2) DataFile");
        BOOST_REQUIRE(second.GetFilePath() == "");
        BOOST_REQUIRE(file_exists("(2) DataFile"));
    }

    // overwrite existing file
    {
        nf::DataFile file("", "DataFile", 0, true, true);
        BOOST_REQUIRE(file.GetFileName() == "DataFile");
        BOOST_REQUIRE(file_exists("DataFile"));
    }


    delete_file("DataFile");
    delete_file("(2) DataFile");
}

void unit_test_append()
{
    delete_file("DataFile");

    // open first file
    {
//        newsflash::DataFile file("", "DataFile", 0, false, false, false);
//        BOOST_REQUIRE(file.name() == "DataFile");
    }

    // open again for append
    {
//        newsflash::DataFile file("", "DataFile", 0, false, false, true);
//        BOOST_REQUIRE(file.name() == "DataFile");
    }

    delete_file("DataFile");
}

void unit_test_discard()
{
    delete_file("DataFile");

    // single threaded discard
    {
        {
            nf::DataFile file("", "DataFile", 0, true, true);
            BOOST_REQUIRE(file_exists("DataFile"));
            file.DiscardOnClose();
        }
        BOOST_REQUIRE(!file_exists("DataFile"));
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
            auto file = std::make_shared<nf::DataFile>("", "DataFile", 0, true, true);

            std::vector<char> buff;
            buff.resize(1024);
            fill_random(&buff[0], buff.size());

            threads.submit(file->Write(0, buff));
            threads.submit(file->Write(0, buff));
            threads.submit(file->Write(0, buff));
            file->DiscardOnClose();
            threads.submit(file->Write(0, buff));
            threads.submit(file->Write(0, buff));
            file.reset();

            threads.wait_all_actions();
            BOOST_REQUIRE(!file_exists("DataFile"));
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