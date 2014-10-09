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
#include "../bodylist.h"
#include "../download_state_machine.h"
#include "../download.h"
#include "../session.h"
#include "../buffer.h"
#include "../action.h"
#include "../ui/download.h"
#include "../ui/settings.h"
#include "unit_test_common.h"

namespace nf = newsflash;
namespace ui = newsflash::ui;

using state = newsflash::ui::task::state;

void unit_test_bodylist()
{
    nf::bodylist list(1, 2, {"alt.binaries.foo", "alt.binaries.bar"}, {"123", "234", "345"});

    BOOST_REQUIRE(list.task() == 1);
    BOOST_REQUIRE(list.account() == 2);    
    BOOST_REQUIRE(!list.is_complete());
    BOOST_REQUIRE(!list.configure_fail());

    // no such group
    {
        std::string command;
        nf::session session;
        session.on_send = [&](const std::string& cmd) {
            command = cmd;
        };

        BOOST_REQUIRE(list.submit_configure_command(0, session));
        BOOST_REQUIRE(command == "GROUP alt.binaries.foo\r\n");

        {
            nf::buffer recv(1024);
            nf::buffer conf;
            recv.append("411 no such group\r\n");
            session.parse_next(recv, conf);
            BOOST_REQUIRE(!list.receive_configure_buffer(0, std::move(conf)));
        }

        BOOST_REQUIRE(list.submit_configure_command(1, session));
        BOOST_REQUIRE(command == "GROUP alt.binaries.bar\r\n");

        {
            nf::buffer recv(1024);
            nf::buffer conf;
            recv.append("411 no such group\r\n");
            session.parse_next(recv, conf);
            BOOST_REQUIRE(!list.receive_configure_buffer(1, std::move(conf)));
        }

        BOOST_REQUIRE(!list.submit_configure_command(2, session));
        BOOST_REQUIRE(list.configure_fail());
    }

    // try again
    list.clear_fail_bits();
    BOOST_REQUIRE(!list.configure_fail());

    {


        std::string command;
        nf::session session;
        session.on_send = [&](const std::string& cmd) {
            command += cmd;
        };


        // configure
        {
            list.submit_configure_command(0, session);

            nf::buffer recv(1024);
            nf::buffer conf;
            recv.append("211 4 1 4 alt.binaries.foo group succesfully selected\r\n");

            session.parse_next(recv, conf);
            BOOST_REQUIRE(list.receive_configure_buffer(0, std::move(conf)));
        }

        command = "";

        session.enable_pipelining(true);

        // send data commands
        {
            list.submit_data_commands(session);
            BOOST_REQUIRE(command == "BODY 123\r\nBODY 234\r\nBODY 345\r\n");

            nf::buffer recv(1024);
            nf::buffer body1, body2, body3;
            recv.append("420 no article with that message\r\n"
                "222 body follows\r\nhello\r\n.\r\n"
                "420 no article with that message\r\n");

            session.parse_next(recv, body1);
            list.receive_data_buffer(std::move(body1));

            session.parse_next(recv, body2);
            list.receive_data_buffer(std::move(body2));

            session.parse_next(recv, body3);
            list.receive_data_buffer(std::move(body3));

            auto buffers = list.get_buffers();
            BOOST_REQUIRE(buffers.size() == 3);

            BOOST_REQUIRE(buffers[0].content_status() == nf::buffer::status::unavailable);
            BOOST_REQUIRE(buffers[2].content_status() == nf::buffer::status::unavailable);
            BOOST_REQUIRE(buffers[1].content_status() == nf::buffer::status::success);
            BOOST_REQUIRE(buffers[1].content_length() == std::strlen("hello\r\n.\r\n"));
        }
    }

}

void unit_test_state_machine()
{
    using state = ui::task::state;

    nf::download_state_machine stm(5);

    BOOST_REQUIRE(stm.get_state() == state::queued);

}

int test_main(int, char*[])
{
    unit_test_bodylist();
    unit_test_state_machine();

    return 0;
}