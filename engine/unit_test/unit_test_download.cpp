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
#  include <boost/lexical_cast.hpp>
#  include <boost/filesystem.hpp>
#include <newsflash/warnpop.h>
#include <thread>
#include "../download.h"
#include "../bodylist.h"
#include "../action.h"
#include "../session.h"
#include "unit_test_common.h"

namespace nf = newsflash;

void unit_test_bodylist()
{
    // no such group
    {
        nf::bodylist list({"alt.binaries.foo", "alt.binaries.bar"}, {"123", "234", "345"});

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

    {

        nf::bodylist list({"alt.binaries.foo", "alt.binaries.bar"}, {"123", "234", "345"});

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

            const auto& buffers = list.get_buffers();
            BOOST_REQUIRE(buffers.size() == 3);

            BOOST_REQUIRE(buffers[0].content_status() == nf::buffer::status::unavailable);
            BOOST_REQUIRE(buffers[2].content_status() == nf::buffer::status::unavailable);
            BOOST_REQUIRE(buffers[1].content_status() == nf::buffer::status::success);
            BOOST_REQUIRE(buffers[1].content_length() == std::strlen("hello\r\n.\r\n"));
        }
    }
}

void unit_test_create_cmds()
{
    std::vector<std::string> articles;
    for (int i=0; i<10000; ++i)
        articles.push_back(boost::lexical_cast<std::string>(i));

    std::size_t cmd_number = 0;

    nf::download download({"alt.binaries.foobar"}, articles, "", "test");
    nf::session session;
    session.on_send = [&](const std::string& cmd) {
        BOOST_REQUIRE(cmd == "BODY " + 
            boost::lexical_cast<std::string>(cmd_number) + "\r\n");
        ++cmd_number;
    };

    session.enable_pipelining(true);

    for (;;)
    {
        auto cmds = download.create_commands();
        if (!cmds)
            break;

        cmds->submit_data_commands(session);
    }
    BOOST_REQUIRE(cmd_number == articles.size());

}

void unit_test_decode_binary()
{
    delete_file("1489406.jpg");

    nf::download download({"alt.binaries.foobar"}, {"1", "2", "3"}, "", "test");
    nf::session session;
    session.on_send = [&](const std::string&) {};

    auto cmdlist = download.create_commands();

    cmdlist->submit_data_commands(session);
    cmdlist->receive_data_buffer(read_file_buffer("test_data/1489406.jpg-001.ync"));
    cmdlist->receive_data_buffer(read_file_buffer("test_data/1489406.jpg-002.ync"));
    cmdlist->receive_data_buffer(read_file_buffer("test_data/1489406.jpg-003.ync"));

    std::vector<std::unique_ptr<nf::action>> actions1;
    std::vector<std::unique_ptr<nf::action>> actions2;
    download.complete(*cmdlist, actions1);

    while (!actions1.empty())
    {
        for (auto& it : actions1)
        {
            it->perform();
            download.complete(*it, actions2);
        }
        actions1 = std::move(actions2);
        actions2 = std::vector<std::unique_ptr<nf::action>>();
    }

    download.finalize();

    const auto& jpg = read_file_contents("1489406.jpg");
    const auto& ref = read_file_contents("test_data/1489406.jpg");
    BOOST_REQUIRE(jpg == ref);

    delete_file("1489406.jpg");    
}

void unit_test_decode_text()
{}

void test_decode_from_files()
{
    // this test case assumes that we've got some raw NNTP dumps somewhere on the disk and then a 
    // reference binary somewhere 
    namespace fs = boost::filesystem;

    //nf::download 
    std::vector<std::string> articles;

    for (auto it = fs::directory_iterator("/tmp/Newsflash/"); it != fs::directory_iterator(); ++it)
    {
        auto entry = *it;
        auto file  = entry.path().string();
        if (file.find(".log") != std::string::npos)
            continue;
        articles.push_back(file);
    }
    nf::download download({"alt.binaries.foo"}, articles, "", "test");
    nf::session ses;
    ses.on_send = [&](const std::string&) {};

    for (std::size_t i=0; i<articles.size();)
    {
        auto cmds = download.create_commands();
        if (!cmds)
            break;
        for (std::size_t j=0; j<cmds->num_data_commands(); ++j, ++i)
        {
            nf::buffer buff = read_file_buffer(articles[i].c_str());
            cmds->receive_data_buffer(std::move(buff));
        }
        std::vector<std::unique_ptr<nf::action>> decodes;
        download.complete(*cmds, decodes);
        for (auto& dec : decodes)
        {
            dec->perform();
            std::vector<std::unique_ptr<nf::action>> writes;            
            download.complete(*dec, writes);
            for (auto& io : writes)
                io->perform();
        }
    }
}

int test_main(int, char*[])
{
    unit_test_bodylist();
    unit_test_create_cmds();
    unit_test_decode_binary();
    unit_test_decode_text();

    //test_decode_from_files();

    return 0;
}