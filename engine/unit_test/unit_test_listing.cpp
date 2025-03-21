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

#include <vector>
#include <string>
#include <fstream>

#include "engine/listing.h"
#include "engine/buffer.h"
#include "engine/cmdlist.h"
#include "unit_test_common.h"

std::vector<std::string> read_file(const char* file)
{
    std::vector<std::string> ret;
    std::ifstream in(file);

    while(in.good())
    {
        std::string line;
        std::getline(in, line);
        if (!line.empty())
            ret.push_back(std::move(line));
    }
    return ret;

}

void unit_test_success()
{
    const char* body =
        "215 group listing follows\r\n"
        "alt.binaries.pictures.graphics.3d 900 800 y\r\n"
        "alt.binaries.movies.divx 321 123 y\r\n"
        "alt.binaries.sounds.mp3    8523443434535555 80 n\r\n"
        ".\r\n";

    newsflash::Buffer i(1024);
    newsflash::Buffer o;
    i.Append(body);

    std::string cmd;

    newsflash::Session session;
    session.SetSendCallback([&](const std::string& c) {
        cmd = c;
    });

    newsflash::Listing listing;
    BOOST_REQUIRE(listing.HasCommands());
    auto cmds = listing.CreateCommands();
    BOOST_REQUIRE(listing.HasCommands() == false);
    BOOST_REQUIRE(cmds->NeedsToConfigure() == false);
    BOOST_REQUIRE(cmds->GetType() == newsflash::CmdList::Type::Listing);

    cmds->SubmitDataCommands(session);
    session.SendNext();
    session.RecvNext(i, o);

    cmds->ReceiveDataBuffer(std::move(o));

    std::vector<std::unique_ptr<newsflash::action>> actions;
    listing.Complete(*cmds, actions);

    BOOST_REQUIRE(listing.HasCommands() == false);

    BOOST_REQUIRE(listing.NumGroups() == 3);
    BOOST_REQUIRE(listing.GetGroup(0).name == "alt.binaries.pictures.graphics.3d");
    BOOST_REQUIRE(listing.GetGroup(0).last == 900);
    BOOST_REQUIRE(listing.GetGroup(0).first == 800);
    BOOST_REQUIRE(listing.GetGroup(1).name == "alt.binaries.movies.divx");
    BOOST_REQUIRE(listing.GetGroup(1).last == 321);
    BOOST_REQUIRE(listing.GetGroup(1).first == 123);
    BOOST_REQUIRE(listing.GetGroup(2).name == "alt.binaries.sounds.mp3");
    BOOST_REQUIRE(listing.GetGroup(2).last == std::uint64_t(8523443434535555));
    BOOST_REQUIRE(listing.GetGroup(2).first == 80);
}

void unit_test_intermediate_callback()
{
    newsflash::Listing::NewsGroup capture;
    newsflash::Listing listing;
    listing.SetProgressCallback([&](const newsflash::Listing::Progress& progress) {
        BOOST_REQUIRE(progress.groups.size() == 1);
        capture = progress.groups[0];
    });

    auto cmds = listing.CreateCommands();

    newsflash::Buffer recv(1024);

    recv.Append("215 group listing ");
    cmds->InspectIntermediateContentBuffer(recv, false);
    listing.Tick();
    BOOST_REQUIRE(capture.last == 0);
    BOOST_REQUIRE(capture.first == 0);
    BOOST_REQUIRE(capture.size  == 0);

    recv.Append("\r\n");
    cmds->InspectIntermediateContentBuffer(recv, false);
    listing.Tick();
    BOOST_REQUIRE(capture.last == 0);
    BOOST_REQUIRE(capture.first == 0);
    BOOST_REQUIRE(capture.size  == 0);

    recv.Append("alt.binaries.pictures.graphics.3d 900 800 y\r\n");
    cmds->InspectIntermediateContentBuffer(recv, false);
    listing.Tick();
    BOOST_REQUIRE(capture.last  == 900);
    BOOST_REQUIRE(capture.first == 800);

    recv.Append("alt.binaries.movies.divx ");
    cmds->InspectIntermediateContentBuffer(recv, false);
    listing.Tick();
    BOOST_REQUIRE(capture.last  == 900);
    BOOST_REQUIRE(capture.first == 800);

    recv.Append("321 123 y\r\n");
    cmds->InspectIntermediateContentBuffer(recv, false);
    listing.Tick();
    BOOST_REQUIRE(capture.last  == 321);
    BOOST_REQUIRE(capture.first == 123);

    recv.Append(".\r\n");
    cmds->InspectIntermediateContentBuffer(recv, false);
    listing.Tick();
    BOOST_REQUIRE(capture.last  == 321);
    BOOST_REQUIRE(capture.first == 123);

}

void unit_test_failure()
{
    // todo:
}

int test_main(int, char* [])
{

    unit_test_success();
    unit_test_intermediate_callback();
    unit_test_failure();

    return 0;
}