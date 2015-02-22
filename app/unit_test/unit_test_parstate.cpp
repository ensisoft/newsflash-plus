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

int test_main(int, char* argv[])
{
    using status = app::ParState::FileState;


    {

        using state = app::ParState::ExecState;        

        std::vector<app::ParState::File> files;

        app::ParState par(files);
        par.clear();
        par.update("Repair is not possible. \"You need 29 more recovery blocks to be able to repair.\"");
        BOOST_REQUIRE(par.getState() == state::Finish);
        BOOST_REQUIRE(par.getMessage() == "You need 29 more recovery blocks to be able to repair.");

    }


    // {

    //     std::vector<app::ParState::File> files;

    //     app::ParState state(files);

    //     state.update("Loading \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+001.par2\".\n");
    //     BOOST_REQUIRE(files[0].state == status::Loading);
    //     BOOST_REQUIRE(files[0].file  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+001.par2");
    //     state.update("No new packets found\n");
    //     BOOST_REQUIRE(files[0].state == status::Loaded);

    //     state.update("Loading \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+002.par2\".\n");        
    //     BOOST_REQUIRE(files[1].state == status::Loading);
    //     BOOST_REQUIRE(files[1].file  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+002.par2");
    //     state.update("Loading: 11.7%\r");
    //     BOOST_REQUIRE(files[1].prog == F32(11.7));
    //     state.update("Loading: 24.7%\r");        
    //     BOOST_REQUIRE(files[1].prog == F32(24.7));        
    //     state.update("Loading: 40.0%\r");        
    //     BOOST_REQUIRE(files[1].prog == F32(40.0));        

    //     state.update("Loading \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+003.par2\".\n");
    //     BOOST_REQUIRE(files[2].state == status::Loading);
    //     BOOST_REQUIRE(files[2].file  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+003.par2");
    //     state.update("Loaded 34 new packets including 16 recovery blocks\n");
    //     BOOST_REQUIRE(files[2].state == status::Loaded);

    //     state.update("There are 49 recoverable files and 0 other files.\n");
    //     state.update("The block size used was 4814660 bytes.\n");
    //     state.update("There are a total of 2023 data blocks.\n");
    //     state.update("The total size of the data files is 9629320860 bytes.\n");

    //     state.update("Verifying source files:\n");

    //     state.update("Scanning: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar\": 00.0%\n");
    //     BOOST_REQUIRE(files[3].file  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar");
    //     BOOST_REQUIRE(files[3].state == status::Scanning);
    //     state.update("Scanning: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar\": 15.0%\r");
    //     BOOST_REQUIRE(files[3].prog == F32(15.0));
    //     state.update("Scanning: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar\": 30.0%\r");
    //     BOOST_REQUIRE(files[3].prog == F32(30.0));        
    //     state.update("Scanning: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar\": 45.0%\r");                        
    //     BOOST_REQUIRE(files[3].prog == F32(45.0));        
    //     state.update("Target: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar\" - damaged. Found 39 of 42 data blocks.\n");
    //     BOOST_REQUIRE(files[3].prog  == F32(45.0));
    //     BOOST_REQUIRE(files[3].state == status::Damaged);

    //     state.update("File: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part47.rar\" - no data found.\n");
    //     BOOST_REQUIRE(files[4].file == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.part47.rar");
    //     BOOST_REQUIRE(files[4].state == status::NoDataFound);
    //     state.update("Target: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part48.rar\" - missing.\n");
    //     BOOST_REQUIRE(files[5].file  == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.part48.rar");
    //     BOOST_REQUIRE(files[5].state == status::Missing);


    //     state.update("Scanning extra files:\n");
    //     state.update("File: \"nt/media/movies/blarh.r00\" - found 1 of 6 data blocks from \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part47.rar\"\n");
    //     BOOST_REQUIRE(files.size() == 6);

    //     state.update("File: \"nt/media/movies/blarh.r00\" - is a match for \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part48.rar\"\n");        
    //     BOOST_REQUIRE(files[5].state == status::Found);
    // }

    {
        // QString file;
        // float done;

        // BOOST_REQUIRE(app::ParState::parseFileProgress(
        //     "Loading: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+001.par2\": 40.0%\n", file, done));
        // BOOST_REQUIRE(file == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.vol000+001.par2");
        // BOOST_REQUIRE(done == F32(40.0));

        // BOOST_REQUIRE(app::ParState::parseFileProgress(
        //     "Scanning: \"The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar\": 15.0%\r", file, done));
        // BOOST_REQUIRE(file == "The.Judge.2014.720p.BluRay.DTS.x264-iNK.part09.rar");
        // BOOST_REQUIRE(done == F32(15.0));

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



    return 0;
}        