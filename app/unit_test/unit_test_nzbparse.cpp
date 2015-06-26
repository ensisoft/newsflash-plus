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
#  include <QCoreApplication>
#  include <QFile>
#  include <QEventLoop>
#  include <newsflash/warnpop.h>
#include <vector>
#include "../nzbparse.h"

void test_sync()
{
    // success
    {
        QFile input("ubuntu-12.04.4-server-amd64.iso.nzb");
        input.open(QIODevice::ReadOnly);
        BOOST_REQUIRE(input.isOpen());

        std::vector<app::NZBContent> content;
        const auto err = app::parseNZB(input, content);
        BOOST_REQUIRE(err == app::NZBError::None);

        // check content.
        BOOST_REQUIRE(content.size() == 2);

        const auto& iso = content[0];
        BOOST_REQUIRE(iso.date == "1392601712");
        BOOST_REQUIRE(iso.poster == "yEncBin@Poster.com (yEncBin)");
        BOOST_REQUIRE(iso.subject == "Test2 - [1/2] - \"ubuntu-12.04.4-server-amd64.iso\" yEnc (1/1855)");
        BOOST_REQUIRE(iso.groups.size() == 1);
        BOOST_REQUIRE(iso.groups[0] == "alt.binaries.test");

        const auto& par = content[1];
        BOOST_REQUIRE(par.date == "1392602495");
        BOOST_REQUIRE(par.subject == "Test2 - [2/2] - \"ubuntu-12.04.4-server-amd64.par2\" yEnc (1/1)"); 
        BOOST_REQUIRE(par.segments.size() == 1);
        BOOST_REQUIRE(par.segments[0] == "<Part1of1.FB4A5EFFD8D842A6953CDA1F2239C743@1392561150.local>");
    }

    // Io error 
    {
        QFile input;
        std::vector<app::NZBContent> content;
        const auto err = app::parseNZB(input, content);

        BOOST_REQUIRE(err == app::NZBError::Io);
    }

    // nzb content error
    {
        QFile input("ubuntu-12.04.4-server-amd64.iso-nzb-error.nzb");
        std::vector<app::NZBContent> content;

        input.open(QIODevice::ReadOnly);      
        BOOST_REQUIRE(input.isOpen());
        const auto err = app::parseNZB(input, content);

        BOOST_REQUIRE(err == app::NZBError::NZB);
    }

    // xml error
    {
        QFile input("ubuntu-12.04.4-server-amd64.iso-xml-error.nzb");
        std::vector<app::NZBContent> content;

        input.open(QIODevice::ReadOnly);      
        BOOST_REQUIRE(input.isOpen());
        const auto err = app::parseNZB(input, content);

        BOOST_REQUIRE(err == app::NZBError::XML);
    }
}


int test_main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_sync();
    return 0;
}