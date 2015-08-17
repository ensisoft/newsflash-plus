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
#include "../media.h"

int test_main(int, char* argv[])
{
    BOOST_REQUIRE(app::findAdultTitle("Boning.The.Mom.Next.Door.XXX.COMPLETE.NTSC.DVDR-DFA") ==
        "Boning.The.Mom.Next.Door");
    BOOST_REQUIRE(app::findAdultTitle("Big.Dick.Gloryholes.4.XXX.NTSC.DVDR-EViLVAULT") == 
        "Big.Dick.Gloryholes.4");
    BOOST_REQUIRE(app::findAdultTitle("Butterfly.2008.XXX.HDTVRiP.x264-REDX") ==
        "Butterfly");
    BOOST_REQUIRE(app::findAdultTitle("Boca.Chica.Blues.2010.XXX.1080p.HDTV.x264-REDX") ==
        "Boca.Chica.Blues");
    BOOST_REQUIRE(app::findAdultTitle("Hotesses.De.Lair.FRENCH.XXX.DVDRiP.x264-TattooLovers") ==
        "Hotesses.De.Lair");

    BOOST_REQUIRE(app::findTVSeriesTitle("The.Killing.S04E06.DVDRip.x264-OSiTV") == "The.Killing");

    return 0;
}