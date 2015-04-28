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
#include <newsflash/test_float.h>
#include <boost/test/minimal.hpp>
#include "../parstate.h"
#include "../paritychecker.h"

int test_main(int, char* argv[])
{
    using status = app::ParityChecker::FileState;
    using type   = app::ParityChecker::FileType;
    using state = app::ParState::ExecState;        

    {

        app::ParityChecker::File file;
        app::ParState state;

        BOOST_REQUIRE(state.update("Loading: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+001.par2\".\n", file));
        BOOST_REQUIRE(file.state == status::Loading);
        BOOST_REQUIRE(file.type  == type::Parity);
        BOOST_REQUIRE(file.name  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+001.par2");
        BOOST_REQUIRE(state.update("Loaded: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+001.par2\" Loaded 10 new packets\n", file));
        BOOST_REQUIRE(file.state == status::Loaded);
        BOOST_REQUIRE(file.name  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+001.par2");
        BOOST_REQUIRE(file.type  == type::Parity);        

        BOOST_REQUIRE(state.update("Loading: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+002.par2\".\n", file));
        BOOST_REQUIRE(file.state == status::Loading);
        BOOST_REQUIRE(file.type  == type::Parity);                
        BOOST_REQUIRE(file.name  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+002.par2");
        BOOST_REQUIRE(state.update("Loaded: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+002.par2\" No new packets found.", file));
        BOOST_REQUIRE(file.state == status::Loaded);
        BOOST_REQUIRE(file.type  == type::Parity);                
        BOOST_REQUIRE(file.name  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+002.par2");        

        BOOST_REQUIRE(!state.update("There are 49 recoverable filed 0 other file", file));
        BOOST_REQUIRE(!state.update("The block size used was 4814660 bytes.\n", file));
        BOOST_REQUIRE(!state.update("There are a total of 2023 data blocks.\n", file));
        BOOST_REQUIRE(!state.update("The total size of the data file 9629320860 bytes.\n", file));
        BOOST_REQUIRE(!state.update("Verifying source files:", file));

        BOOST_REQUIRE(state.update("Target: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part48.rar\" - missing.\n", file));
        BOOST_REQUIRE(file.state == status::Missing);
        BOOST_REQUIRE(file.type  == type::Target);
        BOOST_REQUIRE(file.name  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.part48.rar");        

        BOOST_REQUIRE(state.update("Target: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar\" - damaged. Found 39 of 42 data blocks.\n", file));
        BOOST_REQUIRE(file.state == status::Damaged);
        BOOST_REQUIRE(file.type  == type::Target);
        BOOST_REQUIRE(file.name  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar");        

        BOOST_REQUIRE(!state.update("Scanning extra files:", file));

        BOOST_REQUIRE(state.update("File: \"nt/media/movies/blarh.r00\" - is a match for \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part48.rar\"\n", file));
        BOOST_REQUIRE(file.state == status::Found);
        BOOST_REQUIRE(file.name  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.part48.rar");
        BOOST_REQUIRE(file.type  == type::Target);
    }

    {
        QString file;
        float done;

        BOOST_REQUIRE(app::ParState::parseScanProgress(
            "Loading: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+001.par2\": 40.0%\n", file, done));
        BOOST_REQUIRE(file == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+001.par2");
        BOOST_REQUIRE(done == F32(40.0));

        BOOST_REQUIRE(app::ParState::parseScanProgress(
            "Scanning: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar\": 15.0%\r", file, done));
        BOOST_REQUIRE(file == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar");
        BOOST_REQUIRE(done == F32(15.0));
    }

    {

        QString step;
        float done;
        BOOST_REQUIRE(app::ParState::parseRepairProgress(
            "Constructing Reed Solomon Matrix: 56.0%\n", step, done));
        BOOST_REQUIRE(step == "Constructing Reed Solomon Matrix");
        BOOST_REQUIRE(done == F32(56.0));

        BOOST_REQUIRE(app::ParState::parseRepairProgress(
            "Repairing Reed Solomon Matrix: 56.0%\n", step, done));
        BOOST_REQUIRE(step == "Repairing Reed Solomon Matrix");
        BOOST_REQUIRE(done == F32(56.0));        

        BOOST_REQUIRE(app::ParState::parseRepairProgress(
            "Repairing: 56.8%\n", step, done));
        BOOST_REQUIRE(step == "Repairing");
        BOOST_REQUIRE(done == F32(56.8));

    }

    {
        app::ParityChecker::File file;

        app::ParState par;
        par.clear();
        BOOST_REQUIRE(par.update("Repair is not possible. \"You need 29 more recovery blocks to be able to repair.\"", file) == false);
        BOOST_REQUIRE(par.getState() == state::Finish);
        BOOST_REQUIRE(par.getMessage() == "You need 29 more recovery blocks to be able to repair.");
    }

    return 0;
}        