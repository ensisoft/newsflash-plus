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
#include <QCoreApplication>
#include <QTimer>
#include "../poweroff.h"
#include "unit_test_poweroff.h"


int test_main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    {
        app::Poweroff power;
        test::Poweroff tester;
        QObject::connect(&power, SIGNAL(initPoweroff()), &tester, SLOT(initPoweroff()));

        power.waitDownloads(true);
        power.numPendingTasks(0);
        BOOST_REQUIRE(tester.initPoweroffSignal);

    }

    {
        app::Poweroff power;
        test::Poweroff tester;
        QObject::connect(&power, SIGNAL(initPoweroff()), &tester, SLOT(initPoweroff()));

        power.waitDownloads(true);
        power.waitRepairs(true);
        power.repairEnqueue();        
        power.numPendingTasks(0);
        
        BOOST_REQUIRE(!tester.initPoweroffSignal);
        power.repairReady();
        BOOST_REQUIRE(tester.initPoweroffSignal);

    }    

    {
        app::Poweroff power;
        test::Poweroff tester;
        QObject::connect(&power, SIGNAL(initPoweroff()), &tester, SLOT(initPoweroff()));

        power.waitDownloads(true);
        power.waitRepairs(true);
        power.waitUnpacks(true);

        power.repairEnqueue();  
        power.numPendingArchives(1);
        power.numPendingTasks(1);
        
        BOOST_REQUIRE(!tester.initPoweroffSignal);
        power.repairReady();
        BOOST_REQUIRE(!tester.initPoweroffSignal);
        power.numPendingArchives(0);
        BOOST_REQUIRE(!tester.initPoweroffSignal);        
        power.numPendingTasks(0);
        BOOST_REQUIRE(tester.initPoweroffSignal);                
    }        

    return 0;
}
