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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <boost/test/minimal.hpp>
#include "newsflash/warnpop.h"

#include "engine/cmdlist.h"
#include "engine/session.h"

namespace nf = newsflash;

void unit_test_bodylist()
{
    // no such group
    {
        nf::CmdList::Messages m;
        m.groups  = {"alt.binaries.foo", "alt.binaries.bar"};
        m.numbers = {"123", "234", "345"};

        nf::CmdList list(std::move(m));

        std::string command;
        nf::Session session;
        session.SetSendCallback([&](const std::string& cmd) {
            command = cmd;
        });

        BOOST_REQUIRE(list.SubmitConfigureCommand(0, session));
        session.SendNext();
        BOOST_REQUIRE(command == "GROUP alt.binaries.foo\r\n");

        {
            nf::Buffer recv(1024);
            nf::Buffer conf;
            recv.Append("411 no such group\r\n");
            session.RecvNext(recv, conf);
            BOOST_REQUIRE(!list.ReceiveConfigureBuffer(0, std::move(conf)));
        }

        BOOST_REQUIRE(list.SubmitConfigureCommand(1, session));
        session.SendNext();
        BOOST_REQUIRE(command == "GROUP alt.binaries.bar\r\n");

        {
            nf::Buffer recv(1024);
            nf::Buffer conf;
            recv.Append("411 no such group\r\n");
            session.RecvNext(recv, conf);
            BOOST_REQUIRE(!list.ReceiveConfigureBuffer(1, std::move(conf)));
        }

        BOOST_REQUIRE(!list.SubmitConfigureCommand(2, session));
        BOOST_REQUIRE(!list.IsCancelled());
    }

    {

        nf::CmdList::Messages m;
        m.groups  = {"alt.binaries.foo", "alt.binaries.bar"};
        m.numbers = {"123", "234", "345"};

        nf::CmdList list(std::move(m));

        std::string command;
        nf::Session session;
        session.SetSendCallback([&](const std::string& cmd) {
            command += cmd;
        });


        // configure
        {
            list.SubmitConfigureCommand(0, session);
            session.SendNext();

            nf::Buffer recv(1024);
            nf::Buffer conf;
            recv.Append("211 4 1 4 alt.binaries.foo group succesfully selected\r\n");

            session.RecvNext(recv, conf);
            BOOST_REQUIRE(list.ReceiveConfigureBuffer(0, std::move(conf)));
        }

        command = "";

        session.SetEnablePipelining(true);

        // send data commands
        {
            list.SubmitDataCommands(session);
            session.SendNext();
            BOOST_REQUIRE(command == "BODY 123\r\nBODY 234\r\nBODY 345\r\n");

            nf::Buffer recv(1024);
            nf::Buffer body1, body2, body3;
            recv.Append("420 no article with that message\r\n"
                "222 body follows\r\nhello\r\n.\r\n"
                "420 no article with that message\r\n");

            session.RecvNext(recv, body1);
            list.ReceiveDataBuffer(std::move(body1));

            session.RecvNext(recv, body2);
            list.ReceiveDataBuffer(std::move(body2));

            session.RecvNext(recv, body3);
            list.ReceiveDataBuffer(std::move(body3));

            const auto& buffers = list.GetBuffers();
            BOOST_REQUIRE(buffers.size() == 3);

            BOOST_REQUIRE(buffers[0].GetContentStatus() == nf::Buffer::Status::Unavailable);
            BOOST_REQUIRE(buffers[1].GetContentStatus() == nf::Buffer::Status::Success);
            BOOST_REQUIRE(buffers[2].GetContentStatus() == nf::Buffer::Status::Unavailable);
            BOOST_REQUIRE(buffers[1].GetContentLength() == std::strlen("hello\r\n.\r\n"));
        }
    }
}

bool operator==(const nf::Buffer& buff, const char* str)
{
   BOOST_REQUIRE(buff.GetContentLength() == std::strlen(str));

   return !std::strncmp(buff.Content(), str,
        buff.GetContentLength());
}

void unit_test_refill()
{
    nf::CmdList::Messages m;
    m.groups  = {"alt.binaries.foo"};
    m.numbers = {"1", "2", "3", "4"};

    nf::CmdList list(std::move(m));

    std::string command;
    nf::Session session;
    session.SetSendCallback([&](const std::string& cmd) {
        command += cmd;
    });

    session.SetEnablePipelining(true);

    list.SubmitDataCommands(session);
    session.SendNext();
    BOOST_REQUIRE(command == "BODY 1\r\nBODY 2\r\nBODY 3\r\nBODY 4\r\n");

    nf::Buffer recv(1024);
    nf::Buffer b1, b2, b3, b4;

    recv.Append("222 body follows\r\n"
                "hello\r\n.\r\n"
                "222 body follows\r\n"
                "foo\r\n.\r\n"
                "420 no article with that message\r\n"
                "420 no article with that message\r\n");
    session.RecvNext(recv, b1);
    session.RecvNext(recv, b2);
    session.RecvNext(recv, b3);
    session.RecvNext(recv, b4);

    list.ReceiveDataBuffer(std::move(b1));
    list.ReceiveDataBuffer(std::move(b2));
    list.ReceiveDataBuffer(std::move(b3));
    list.ReceiveDataBuffer(std::move(b4));
    BOOST_REQUIRE(list.HasFailedContent());

    using status = nf::Buffer::Status;

    {
        const auto& buffers = list.GetBuffers();
        BOOST_REQUIRE(buffers[0].GetContentStatus() == status::Success);
        BOOST_REQUIRE(buffers[1].GetContentStatus() == status::Success);
        BOOST_REQUIRE(buffers[2].GetContentStatus() == status::Unavailable);
        BOOST_REQUIRE(buffers[3].GetContentStatus() == status::Unavailable);
    }

    command = "";
    list.SubmitDataCommands(session);
    session.SendNext();
    BOOST_REQUIRE(command == "BODY 3\r\nBODY 4\r\n");

    recv.Clear();
    recv.Append("222 body follows\r\n"
                "adrvardk\r\n.\r\n"
                "420 no article with that message dmca\r\n");
    b3 = nf::Buffer();
    b4 = nf::Buffer();
    session.RecvNext(recv, b3);
    session.RecvNext(recv, b4);

    list.ReceiveDataBuffer(std::move(b3));
    list.ReceiveDataBuffer(std::move(b4));
    BOOST_REQUIRE(list.HasFailedContent());

    {
        const auto& buffers = list.GetBuffers();
        BOOST_REQUIRE(buffers.size() == 4);
        BOOST_REQUIRE(buffers[0].GetContentStatus() == status::Success);
        BOOST_REQUIRE(buffers[1].GetContentStatus() == status::Success);
        BOOST_REQUIRE(buffers[2].GetContentStatus() == status::Success);
        BOOST_REQUIRE(buffers[3].GetContentStatus() == status::Dmca);

        BOOST_REQUIRE(buffers[0] == "hello\r\n.\r\n");
        BOOST_REQUIRE(buffers[1] == "foo\r\n.\r\n");
        BOOST_REQUIRE(buffers[2] == "adrvardk\r\n.\r\n");
    }

    command = "";
    list.SubmitDataCommands(session);
    session.SendNext();
    BOOST_REQUIRE(command == "BODY 4\r\n");

    recv.Clear();
    recv.Append("222 body follows\r\n"
        "foobar\r\n.\r\n");
    b4 = nf::Buffer();
    session.RecvNext(recv, b4);

    list.ReceiveDataBuffer(std::move(b4));
    BOOST_REQUIRE(list.HasFailedContent() == false);

    {
        const auto& buffers = list.GetBuffers();
        BOOST_REQUIRE(buffers.size() == 4);
        BOOST_REQUIRE(buffers[0].GetContentStatus() == status::Success);
        BOOST_REQUIRE(buffers[1].GetContentStatus() == status::Success);
        BOOST_REQUIRE(buffers[2].GetContentStatus() == status::Success);
        BOOST_REQUIRE(buffers[3].GetContentStatus() == status::Success);

        BOOST_REQUIRE(buffers[0] == "hello\r\n.\r\n");
        BOOST_REQUIRE(buffers[1] == "foo\r\n.\r\n");
        BOOST_REQUIRE(buffers[2] == "adrvardk\r\n.\r\n");
        BOOST_REQUIRE(buffers[3] == "foobar\r\n.\r\n");
    }


}

void unit_test_listing()
{
    nf::CmdList listing(nf::CmdList::Listing{});

    std::string command;
    nf::Session session;
    session.SetSendCallback([&](const std::string& cmd) {
        command = cmd;
    });

    listing.SubmitDataCommands(session);

    session.SendNext();
    BOOST_REQUIRE(command == "LIST\r\n");

    nf::Buffer i(1024);
    nf::Buffer o(1024);
    i.Append("215 listing follows\r\n"
        "alt.binaries.foo 1 0 y\r\n"
        "alt.binaries.bar 2 1 n\r\n"
        ".\r\n");
    session.RecvNext(i, o);
    BOOST_REQUIRE(o.GetContentStatus() == nf::Buffer::Status::Success);

    listing.ReceiveDataBuffer(std::move(o));

    const auto& buffers = listing.GetBuffers();
    BOOST_REQUIRE(buffers.size() == 1);
    BOOST_REQUIRE(buffers[0].GetContentStatus() == nf::Buffer::Status::Success);

}

void unit_test_groupinfo()
{
    // success
    {
        nf::CmdList list(nf::CmdList::GroupInfo{"alt.binaries.foo"});

        std::string command;
        nf::Session session;
        session.SetSendCallback([&](const std::string& cmd) {
            command = cmd;
        });

        BOOST_REQUIRE(list.NeedsToConfigure() == false);

        list.SubmitDataCommands(session);
        session.SendNext();

        BOOST_REQUIRE(command == "GROUP alt.binaries.foo\r\n");

        nf::Buffer i(64);
        nf::Buffer o(64);
        i.Append("211 3 0 2 alt.binaries.foo\r\n");

        session.RecvNext(i, o);
        BOOST_REQUIRE(o.GetContentStatus() == nf::Buffer::Status::Success);

        list.ReceiveDataBuffer(std::move(o));

        const auto& buffers = list.GetBuffers();
        BOOST_REQUIRE(buffers.size() == 1);
        BOOST_REQUIRE(buffers[0].GetContentStatus() == nf::Buffer::Status::Success);
    }

    // failure
    {
        nf::CmdList list(nf::CmdList::GroupInfo{"alt.binaries.foo"});

        std::string command;
        nf::Session session;
        session.SetSendCallback([&](const std::string& cmd) {
            command = cmd;
        });

        list.SubmitDataCommands(session);

        session.SendNext();

        nf::Buffer i(64);
        nf::Buffer o(64);
        i.Append("411 no such newsgroup\r\n");
        session.RecvNext(i, o);

        list.ReceiveDataBuffer(std::move(o));

        const auto& buffers = list.GetBuffers();
        BOOST_REQUIRE(buffers.size() == 1);
        BOOST_REQUIRE(buffers[0].GetContentStatus() == nf::Buffer::Status::Unavailable);

    }
}

int test_main(int, char*[])
{
    unit_test_bodylist();
    unit_test_refill();
    unit_test_listing();
    unit_test_groupinfo();
    return 0;
}