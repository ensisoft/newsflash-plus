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
#include "../unrar.h"

int test_main(int, char* argv[])
{
    {
        QString name;
        BOOST_REQUIRE(app::Unrar::parseVolume(
            "Extracting from sonido-hawking-1080p.r00", name));
        BOOST_REQUIRE(name == "sonido-hawking-1080p.r00");

        BOOST_REQUIRE(app::Unrar::parseVolume(
            "Extracting from Cold.in.July.2014.1080p.BluRay.DTS.x264-CyTSuNee.part02.rar", name));
        BOOST_REQUIRE(name == "Cold.in.July.2014.1080p.BluRay.DTS.x264-CyTSuNee.part02.rar");

        int done;
        BOOST_REQUIRE(app::Unrar::parseProgress(
             "...         sonido-hawking-1080p.mkv                                    ....  4....  5",
             name, done));
        BOOST_REQUIRE(name == "sonido-hawking-1080p.mkv");
        BOOST_REQUIRE(done == 4);
        
    }


    {
        QString message;
        bool success = false;
        BOOST_REQUIRE(app::Unrar::parseTermination(
            "ERROR: Bad archive Cold.in.July.2014.1080p.BluRay.DTS.x264-CyTSuNee.part02.rar", message, success));
        BOOST_REQUIRE(message == "ERROR: Bad archive Cold.in.July.2014.1080p.BluRay.DTS.x264-CyTSuNee.part02.rar");
        BOOST_REQUIRE(success == false);

    }

    {
        QString message;
        BOOST_REQUIRE(app::Unrar::parseMessage(
            "Chugginton.S04E10.DVDRip.x264-KiDDoS.mkv : packed data checksum error in volume chugginton.s04e10.dvdrip.x264-kiddos.r00",
            message));
        BOOST_REQUIRE(message == "packed data checksum error in volume chugginton.s04e10.dvdrip.x264-kiddos.r00");
    }

    {
        QStringList files;
        files << "sonido-hawking-1080p.part02.rar";        
        files << "sonido-hawking-1080p.part03.rar";                
        files << "sonido-hawking-1080p.part02.rar";        
        files << "sonido-hawking-1080p.part01.rar";        

        QStringList ret = app::Unrar::findVolumes(files);
        BOOST_REQUIRE(ret.size() == 1);        
        BOOST_REQUIRE(ret[0] == "sonido-hawking-1080p.part01.rar");

        files << "terminator2.720p.r00";
        files << "terminator2.720p.r01";
        files << "terminator2.720p.r03";
        files << "terminator2.720p.r02";                        
        ret = app::Unrar::findVolumes(files);
        BOOST_REQUIRE(ret.size() == 2);
        BOOST_REQUIRE(ret[0] == "sonido-hawking-1080p.part01.rar");        
        BOOST_REQUIRE(ret[1] == "terminator2.720p.r00");

        files << "terminator2.720p.rar";
        ret = app::Unrar::findVolumes(files);
        BOOST_REQUIRE(ret.size() == 2);
        BOOST_REQUIRE(ret[0] == "sonido-hawking-1080p.part01.rar");        
        BOOST_REQUIRE(ret[1] == "terminator2.720p.rar");        
    }


    return 0;
}