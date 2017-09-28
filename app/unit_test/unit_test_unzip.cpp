// Copyright (c) 2010-2017 Sami Väisänen, Ensisoft
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

#include "app/unzip.h"

int test_main(int, char*[])
{
    // the example output from 7za is something like this
    // note that when the extraction happens the line containing the progress is rewritten.
    //
    //
    // [enska@arch test]$ 7za e -aoa foo.7z
    //
    // 7-Zip (a) [64] 16.02 : Copyright (c) 1999-2016 Igor Pavlov : 2016-05-21
    // p7zip Version 16.02 (locale=en_US.UTF-8,Utf16=on,HugeFiles=on,64 bits,4 CPUs Intel(R) Core(TM) i5-6600K CPU @ 3.50GHz (506E3),ASM,AES-NI)
    //
    // Scanning the drive for archives:
    // 1 file, 107116940 bytes (103 MiB)
    //
    // Extracting archive: foo.7z
    // --
    // Path = foo.7z
    // Type = 7z
    // Physical Size = 107116940
    // Headers Size = 593
    // Method = LZMA:24
    // Solid = +
    // Blocks = 1
    //
    //  53% 6 - Sabaton - The Last Stand 2016/07.The Last Stand.mp3
    //
    // Everything is Ok
    //
    // Folders: 1
    // Files: 14
    // Size:       108043322
    // Compressed: 107116940

    {
        QString file;
        BOOST_REQUIRE(app::Unzip::parseVolume("Extracting archive: foo.7z", file));
        BOOST_REQUIRE(file == "foo.7z");
        BOOST_REQUIRE(app::Unzip::parseVolume("Extracting archive: Sabbaton - The Last Stand 2016.7z", file));
        BOOST_REQUIRE(file == "Sabbaton - The Last Stand 2016.7z");
    }

    // actually this doesn't work. it seems that the 7za realizes when it's being
    // attached to a tty and when it isn't and modifies its output accordingly
    // thus when we invoke the 7za it doesn't print file/progress information.
    {
        QString file;
        int done;
        BOOST_REQUIRE(app::Unzip::parseProgress("53% 6 - Sabaton - The Last Stand 2016/07.The Last Stand.mp3", file, done));
        BOOST_REQUIRE(file == "6 - Sabaton - The Last Stand 2016/07.The Last Stand.mp3");
        BOOST_REQUIRE(done == 53);
    }

    {
        QString msg;
        BOOST_REQUIRE(app::Unzip::parseMessage("Everything is Ok", msg));
        BOOST_REQUIRE(msg == "Everything is Ok");
        BOOST_REQUIRE(app::Unzip::parseMessage("Open ERROR: Can not open the file as [7z] archive", msg));
        BOOST_REQUIRE(msg == "Open ERROR: Can not open the file as [7z] archive");
    }


    return 0;
}