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
#include <newsflash/warnpush.h>
#  include <boost/test/minimal.hpp>
#include <newsflash/warnpop.h>

#include "../cmdlist.h"
#include "../session.h"

namespace nf = newsflash;

void unit_test_bodylist()
{
    // no such group
    {
        nf::cmdlist list({"alt.binaries.foo", "alt.binaries.bar"}, {"123", "234", "345"}, 
            nf::cmdlist::type::body);

        std::string command;
        nf::session session;
        session.on_send = [&](const std::string& cmd) {
            command = cmd;
        };

        BOOST_REQUIRE(list.submit_configure_command(0, session));
        session.send_next();
        BOOST_REQUIRE(command == "GROUP alt.binaries.foo\r\n");

        {
            nf::buffer recv(1024);
            nf::buffer conf;
            recv.append("411 no such group\r\n");
            session.recv_next(recv, conf);
            BOOST_REQUIRE(!list.receive_configure_buffer(0, std::move(conf)));
        }

        BOOST_REQUIRE(list.submit_configure_command(1, session));
        session.send_next();
        BOOST_REQUIRE(command == "GROUP alt.binaries.bar\r\n");

        {
            nf::buffer recv(1024);
            nf::buffer conf;
            recv.append("411 no such group\r\n");
            session.recv_next(recv, conf);
            BOOST_REQUIRE(!list.receive_configure_buffer(1, std::move(conf)));
        }

        BOOST_REQUIRE(!list.submit_configure_command(2, session));
        BOOST_REQUIRE(!list.is_canceled());
    }

    {

        nf::cmdlist list({"alt.binaries.foo", "alt.binaries.bar"}, {"123", "234", "345"},
            nf::cmdlist::type::body);

        std::string command;
        nf::session session;
        session.on_send = [&](const std::string& cmd) {
            command += cmd;
        };


        // configure
        {
            list.submit_configure_command(0, session);
            session.send_next();

            nf::buffer recv(1024);
            nf::buffer conf;
            recv.append("211 4 1 4 alt.binaries.foo group succesfully selected\r\n");

            session.recv_next(recv, conf);
            BOOST_REQUIRE(list.receive_configure_buffer(0, std::move(conf)));
        }

        command = "";

        session.enable_pipelining(true);

        // send data commands
        {
            list.submit_data_commands(session);
            session.send_next();
            BOOST_REQUIRE(command == "BODY 123\r\nBODY 234\r\nBODY 345\r\n");

            nf::buffer recv(1024);
            nf::buffer body1, body2, body3;
            recv.append("420 no article with that message\r\n"
                "222 body follows\r\nhello\r\n.\r\n"
                "420 no article with that message\r\n");

            session.recv_next(recv, body1);
            list.receive_data_buffer(std::move(body1));

            session.recv_next(recv, body2);
            list.receive_data_buffer(std::move(body2));

            session.recv_next(recv, body3);
            list.receive_data_buffer(std::move(body3));

            const auto& buffers = list.get_buffers();
            BOOST_REQUIRE(buffers.size() == 3);

            BOOST_REQUIRE(buffers[0].content_status() == nf::buffer::status::unavailable);
            BOOST_REQUIRE(buffers[2].content_status() == nf::buffer::status::unavailable);
            BOOST_REQUIRE(buffers[1].content_status() == nf::buffer::status::success);
            BOOST_REQUIRE(buffers[1].content_length() == std::strlen("hello\r\n.\r\n"));
        }
    }
}

bool operator==(const nf::buffer& buff, const char* str)
{
    BOOST_REQUIRE(buff.content_length() == std::strlen(str));

    return !std::strncmp(buff.content(), str,
        buff.content_length());
}

void unit_test_refill()
{
    nf::cmdlist list({"alt.binaries.foo"}, {"1", "2", "3", "4"},
        nf::cmdlist::type::body);

    std::string command;
    nf::session session;
    session.on_send = [&](const std::string& cmd) {
        command += cmd;
    };

    session.enable_pipelining(true);

    list.submit_data_commands(session);
    session.send_next();
    BOOST_REQUIRE(command == "BODY 1\r\nBODY 2\r\nBODY 3\r\nBODY 4\r\n");

    nf::buffer recv(1024);
    nf::buffer b1, b2, b3, b4;

    recv.append("222 body follows\r\n"
                "hello\r\n.\r\n"
                "222 body follows\r\n"
                "foo\r\n.\r\n"
                "420 no article with that message\r\n"
                "222 body follows\r\n"
                "bar\r\n.\r\n");
    session.recv_next(recv, b1);
    session.recv_next(recv, b2);
    session.recv_next(recv, b3);
    session.recv_next(recv, b4);            

    list.receive_data_buffer(std::move(b1));
    list.receive_data_buffer(std::move(b2));
    list.receive_data_buffer(std::move(b3));
    list.receive_data_buffer(std::move(b4));            

    command = "";
    list.submit_data_commands(session);
    session.send_next();
    BOOST_REQUIRE(command == "BODY 3\r\n");

    recv.clear();
    recv.append("222 body follows\r\n"
                "adrvardk\r\n.\r\n");
    b3 = nf::buffer();
    session.recv_next(recv, b3);

    list.receive_data_buffer(std::move(b3));

    using status = nf::buffer::status;

    const auto& buffers = list.get_buffers();
    BOOST_REQUIRE(buffers.size() == 4);
    BOOST_REQUIRE(buffers[0].content_status() == status::success);
    BOOST_REQUIRE(buffers[1].content_status() == status::success);
    BOOST_REQUIRE(buffers[2].content_status() == status::success);
    BOOST_REQUIRE(buffers[3].content_status() == status::success);        
    
    BOOST_REQUIRE(buffers[0] == "hello");


}

int test_main(int, char*[])
{
    unit_test_bodylist();
    unit_test_refill();
    return 0;
}