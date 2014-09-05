// Copyright (c) 2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <newsflash/config.h>

#include <boost/test/minimal.hpp>
#include <thread>
#include "../download.h"
#include "../cmdlist.h"
#include "../session.h"
#include "../buffer.h"
#include "../action.h"
#include "../ui/download.h"
#include "../ui/settings.h"
#include "unit_test_common.h"


namespace nf = newsflash;
namespace ui = newsflash::ui;

using state = newsflash::ui::task::state;

void set(nf::buffer& buff, const char* str)
{
    buff.clear();
    std::strcpy(buff.back(), str);
    buff.append(std::strlen(str));
}

void unit_test_state()
{
    ui::download details;
    details.articles.push_back("1");
    details.desc = "test";
    details.groups.push_back("alt.binaries.foo");
    details.path = "./";
    details.size = 1024;    

    nf::download download(123, 321, 111, details);    

    auto ui = download.get_ui_state();
    BOOST_REQUIRE(ui.st == state::queued);
    BOOST_REQUIRE(ui.id == 123);
    BOOST_REQUIRE(ui.batch == 321);
    BOOST_REQUIRE(ui.desc == "test");
    BOOST_REQUIRE(ui.size == 1024);
    BOOST_REQUIRE(ui.runtime == 0);
    BOOST_REQUIRE(ui.etatime == 0);
    BOOST_REQUIRE(ui.errors.value() == 0);

    download.pause();
    BOOST_REQUIRE(download.get_state() == state::paused);

    download.resume();
    BOOST_REQUIRE(download.get_state() == state::queued);

    download.start();
    BOOST_REQUIRE(download.get_state() == state::waiting);

    ui = download.get_ui_state();
    auto runtime = ui.runtime;
    auto etatime = ui.etatime;
    BOOST_REQUIRE(runtime != 0);    
  //BOOST_REQUIRE(etatime != 0);

    download.pause();
    BOOST_REQUIRE(download.get_state() == state::paused);
    ui = download.get_ui_state();
    runtime = ui.runtime;
    etatime = ui.etatime;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ui = download.get_ui_state();
    BOOST_REQUIRE(ui.runtime == runtime);
    BOOST_REQUIRE(ui.etatime == etatime);

    download.resume();
    BOOST_REQUIRE(download.get_state() == state::waiting);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ui = download.get_ui_state();
    BOOST_REQUIRE(ui.runtime > runtime);


}

void unit_test_cmdlist()
{
    std::string command;
    nf::session session;
    session.on_send = [&](const std::string& cmd) {
        command = cmd;
    };
    session.on_auth = [](std::string& user, std::string& pass) {
        user = "roger";
        pass = "rabbit";
    };

    ui::download details;
    details.articles.push_back("1");
    details.articles.push_back("2");
    details.articles.push_back("3");
    details.articles.push_back("4");    
    details.desc = "test";
    details.groups.push_back("alt.binaries.foo");
    details.groups.push_back("alt.binaries.bar");    
    details.path = "./";
    details.size = 1024;

    // configure fails, neither group exists
    {
        nf::download download(123, 321, 111, details);
        download.on_action = [](std::unique_ptr<nf::action> a) {};
        download.on_error  = [](const ui::error&) { 
            BOOST_FAIL("unexpected error"); 
        };

        nf::buffer recvbuf(1024);
        nf::buffer content(1024);

        std::unique_ptr<nf::cmdlist> cmds;
        BOOST_REQUIRE(download.get_next_cmdlist(cmds));
        BOOST_REQUIRE(cmds->account() == 111);
        BOOST_REQUIRE(cmds->task() == 123);
        BOOST_REQUIRE(!cmds->is_done(nf::cmdlist::step::configure));

        cmds->submit(nf::cmdlist::step::configure, session);
        BOOST_REQUIRE(command == "GROUP alt.binaries.foo\r\n");

        set(recvbuf, "411 no such group\r\n");
        session.parse_next(recvbuf, content);
        cmds->receive(nf::cmdlist::step::configure, content);
        BOOST_REQUIRE(!cmds->is_done(nf::cmdlist::step::configure));

        cmds->next(nf::cmdlist::step::configure);
        cmds->submit(nf::cmdlist::step::configure, session);
        BOOST_REQUIRE(command == "GROUP alt.binaries.bar\r\n");

        set(recvbuf, "411 no such group\r\n");
        session.parse_next(recvbuf, content);
        cmds->receive(nf::cmdlist::step::configure, content);
        cmds->next(nf::cmdlist::step::configure);

        BOOST_REQUIRE(cmds->is_done(nf::cmdlist::step::configure));

        download.complete(std::move(cmds));
        auto ui = download.get_ui_state();
        BOOST_REQUIRE(ui.st == state::complete);
        BOOST_REQUIRE(ui.errors.test(ui::task::flags::unavailable));
    }
}


int test_main(int, char*[])
{
    unit_test_state();
    unit_test_cmdlist();

    return 0;
}