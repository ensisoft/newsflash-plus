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

#include "engine/update.h"
#include "engine/cmdlist.h"
#include "engine/session.h"
#include "engine/buffer.h"
#include "engine/nntp.h"
#include "engine/filesys.h"
#include "engine/filemap.h"
#include "engine/catalog.h"
#include "engine/index.h"
#include "engine/idlist.h"
#include "unit_test_common.h"

void unit_test_ranges()
{
    delete_file("alt.binaries.test/alt.binaries.test.nfo");

    fs::createpath("alt.binaries.test");

    std::string str;
    newsflash::Session session;
    session.on_send = [&](const std::string& c) {
        str += c;
    };
    session.SetEnablePipelining(true);

    std::vector<std::unique_ptr<newsflash::action>> actions;

    {
        newsflash::Update u("", "alt.binaries.test");

        auto cmd = u.CreateCommands();

        newsflash::buffer buff(1024);
        buff.append("211 10 1 1500 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 10 1 1500 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->ReceiveDataBuffer(buff);

        u.Complete(*cmd, actions);
        BOOST_REQUIRE(u.HasCommands());

        str.clear();

        cmd = u.CreateCommands();
        cmd->SubmitDataCommands(session);
        session.SendNext();
        BOOST_REQUIRE(str == "XOVER 501-1500\r\nXOVER 1-500\r\n");

    }

    std::ofstream out("alt.binaries.test/alt.binaries.test.nfo", std::ios::out | std::ios::binary | std::ios::trunc);
    //out << "1045" << std::endl;
    //out << "2048" << std::endl;
    std::uint64_t val;
    std::uint8_t val8;
    val8 = 1; // version
    out.write((const char*)&val8, sizeof(val8));
    val = 1045; // local first
    out.write((const char*)&val, sizeof(val));
    val = 2048; // local last
    out.write((const char*)&val, sizeof(val));
    val = 0; // landmark, positive and negative offset
    out.write((const char*)&val, sizeof(val));
    out.write((const char*)&val, sizeof(val));
    out.write((const char*)&val, sizeof(val));
    out.close();

    // newer items
    {
        newsflash::Update u("", "alt.binaries.test");

        auto cmd = u.CreateCommands();

        newsflash::buffer buff(1024);
        buff.append("211 3500 1045 3052 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 3500 1045 3052 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->ReceiveDataBuffer(buff);

        u.Complete(*cmd, actions);

        str.clear();

        cmd = u.CreateCommands();
        cmd->SubmitDataCommands(session);
        session.SendNext();
        BOOST_REQUIRE(str == "XOVER 2049-3048\r\nXOVER 3049-3052\r\n");

    }


    // older items
    {
        newsflash::Update u("", "alt.binaries.test");

        auto cmd = u.CreateCommands();

        newsflash::buffer buff(1024);
        buff.append("211 3500 348 2048 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 3500 348 2048 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->ReceiveDataBuffer(buff);

        u.Complete(*cmd, actions);
        str.clear();

        cmd = u.CreateCommands();
        cmd->SubmitDataCommands(session);
        session.SendNext();
        BOOST_REQUIRE(str == "XOVER 348-1044\r\n");
    }

    // no new items
    {
        newsflash::Update u("", "alt.binaries.test");

        auto cmd = u.CreateCommands();

        newsflash::buffer buff(1024);
        buff.append("211 3500 1045 2048 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 3500 1045 2048 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->ReceiveDataBuffer(buff);

        u.Complete(*cmd, actions);
        str.clear();

        BOOST_REQUIRE(u.HasCommands() == false);
    }

    delete_file("alt.binaries.test/alt.binaries.test.nfo");
}

void unit_test_data()
{
    std::vector<std::unique_ptr<newsflash::action>> actions;

    fs::createpath("alt.binaries.test");
    delete_file("alt.binaries.test/vol000000000000000.dat");
    delete_file("alt.binaries.test/alt.binaries.test.nfo");
    delete_file("alt.binaries.test/alt.binaries.test.idb");

    // create from initial data
    {

        newsflash::Update u("", "alt.binaries.test");

        auto cmd = u.CreateCommands();

        newsflash::buffer buff(1024 * 10);
        buff.append("211 49999 1 500000 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 49999 1 500000 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->ReceiveDataBuffer(buff);

        u.Complete(*cmd, actions);

        cmd = u.CreateCommands();

        std::string str;

        nntp::overview ov = {};
        ov.author.start    = "ensi@gmail.com";
        ov.subject.start   = "Metallica - Enter Sandman yEnc (01/10).mp3";
        ov.bytecount.start = "1024";
        ov.date.start      = "Tue, 13 May 2006 00:00:00";
        ov.number.start    = "5";
        ov.messageid.start = "<5>";
        str += nntp::make_overview(ov);

        ov.author.start    = "ensi@gmail.com";
        ov.subject.start   = "Metallica - Enter Sandman yEnc (02/10).mp3";
        ov.bytecount.start = "568";
        ov.date.start      = "Tue, 13 May 2006 00:00:00";
        ov.number.start    = "2";
        ov.messageid.start = "<2>";
        str += nntp::make_overview(ov);

        ov.author.start    = "ensi@gmail.com";
        ov.subject.start   = "Metallica - Enter Sandman yEnc (03/10).mp3";
        ov.bytecount.start = "100";
        ov.date.start      = "Tue, 13 May 2006 00:00:00";
        ov.number.start    = "7";
        ov.messageid.start = "<7>";
        str += nntp::make_overview(ov);

        ov.author.start    = "foo@gmail.com";
        ov.subject.start   = ".net and COM interoperability";
        ov.bytecount.start = "512";
        ov.date.start      = "Tue, 15 Apr 2004 00:00:00";
        ov.number.start    = "454";
        ov.messageid.start = "<454>";
        str += nntp::make_overview(ov);

        ov.author.start    = "ano@anonymous.yy (knetje)";
        ov.subject.start   = "#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (1/4)";
        ov.bytecount.start = "2048";
        ov.date.start      = "Tue, 13 Jun 2008 00:00:00";
        ov.number.start    = "30";
        ov.messageid.start = "<30>";
        str += nntp::make_overview(ov);

        ov.author.start    = "ano@anonymous.yy (knetje)";
        ov.subject.start   = "#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (2/4)";
        ov.bytecount.start = "2048";
        ov.date.start      = "Tue, 13 Jun 2008 00:00:00";
        ov.number.start    = "40";
        ov.messageid.start = "<40>";
        str += nntp::make_overview(ov);

        ov.author.start    = "ano@anonymous.yy (knetje)";
        ov.subject.start   = "#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (3/4)";
        ov.bytecount.start = "2048";
        ov.date.start      = "Tue, 13 Jun 2008 00:00:00";
        ov.number.start    = "50";
        ov.messageid.start = "<50>";
        str += nntp::make_overview(ov);

        ov.author.start    = "foo@acme.com";
        ov.subject.start   = "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (102).jpg (1/1)";
        ov.bytecount.start = "768";
        ov.date.start      = "Tue, 13 Jun 2008 00:00:00";
        ov.number.start    = "100";
        ov.messageid.start = "<100>";
        str += nntp::make_overview(ov);

        ov.author.start    = "foo@acme.com";
        ov.subject.start   = "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (103).jpg (1/1)";
        ov.bytecount.start = "100";
        ov.date.start      = "Tue, 13 Jun 2008 00:00:00";
        ov.number.start    = "101";
        ov.messageid.start = "<101>";
        str += nntp::make_overview(ov);

        ov.author.start    = "foo@acme.com";
        ov.subject.start   = "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (104).jpg (2/2)";
        ov.bytecount.start = "100";
        ov.date.start      = "Tue, 13 Jun 2008 00:00:00";
        ov.number.start    = "102";
        ov.messageid.start = "<102>";
        str += nntp::make_overview(ov);

        // these items are not valid and should be discarded.
        ov.author.start    = "foo@acme.com";
        ov.subject.start   = "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (105).jpg (7/4)";
        ov.bytecount.start = "100";
        ov.date.start      = "Tue, 13 Jun 2008 00:00:00";
        ov.number.start    = "103";
        ov.messageid.start = "<103>";
        str += nntp::make_overview(ov);

        ov.author.start = "test@test.com";
        ov.subject.start = "ange___________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "__________________________________________________________________________________________________________________"
        "____________________________________________________________________________________________________LSYFFpdxSeKdmx"
        "tRcCtjijKHDrPbPOIXfnYRHhQqEPDdQpMTyWUUXNxgBbkISHItSHsOfomcUOKbwyCJufvUuyohTmsfFFjFEHLzlWgYpIySvkyXyEaIqxbAlVTPUujH"
        "RBkCMMVUAtBmCtSfbfpOOcapFPJSNIPRXjaxZyyRnRWqFxaIVewChNLkOApXtJVOmrOdBlmpIGwEDYRjmJxwpzWNluGhqelHwuqNcSIVdRSVsAxkXZ"
        "lGzOEHYikdpLvTCTCejvQtUkiuboauCRBXMgHAGwzcavCNawOozLpuLDrTFdOxZaBOUrTmUBYwGKZDMweYNfaySRrFMWhkWvHKPzfXDuTmhIMqHLZC"
        "CijxhZvklmSYvtgZEKbUdjvPYOcaJMhjfYnZgDcJTJsYAJFTHRqijURxJcxfDbIxWqIHlltfMwSjSjZajIIwvBsgxomkrKtcTTGCLPuMeAHbvPApJi"
        "fjhMPQWJhBJTAhjxlVATlJOwTBmdaGuAWFqwCwfEhkCmNwUNuhDBJskxPTOcNLnLDYtWpjpyuIxGMdCUamnlevbOrPPtriJjlpIBTkYYrvmHpYxoIk"
        "VyOuGeXJDKDZtjCOshItyiSHoeRpmcNcgDhgpmTTjHresOqxFygMKFgjKdEYeeAzzoyrxpjqNiIZeUbFfSPsBfkKYVEtzlnkUvtYATzLeTyKKHgFob"
        "nHAMaFCmJkWaFpuKVZkHiYDcghyGdfXSjmkfXvTMTLnNZpNBYsdgpHTIyjuzPogJAdIRfYOTmAVSKuZHHlFdemcrmgUiMiCvtOpxtaAMwORCLhCRdy"
        "QDJqoJxmfpwPZPGkEhXmhLuSvwulWhgzVgbxjngoiVczKGLmcAIWEFvEHAoPCVQDsrHsvpAhiKanjTVkNJaXfgbHyHsrvSPmKJJXsIrYdShVojBvcE"
        "MZkjjkSyyvaRBopNGjdFfgveYkFPUxYwsBinTcbZqzufJehNmhqyICbRLLrYowIMtksMLKHgpvPNwvlUmRwtHXrGTLuAiPzDZcQQvbJdfwwnDqjzCD"
        "ACEVmHRnfYxmXWKJgXugPPeKNCfGtrjDaSnCFDAbhmtXzCJuvpZqRMzfuyxZGGlgFQwtRYOadkfecdpdWixpGfodgwUJEVKldlXAvycbGcYYuVHErG"
        "XVVagjqcCPvtxEzcjAMWgrYZrvRxKygoEHfMpRfpJJvSTOsrYqBUjZSuoivdfSEjbSAOhnUvbUqwaLKbQtyfrHPYNcHdZEIoZpsWnfnXkxhrLVemXf"
        "zlpflePfVxeibNbHGzAEfmznTehqznzLsdOVrXWVZVtcBpJgtxBEMQkrnbJrtxxHcdWNjEUPcMPrnBxWDMGoWWmeDGnWBxHQJAGoFWVaiOAjqfYSem"
        "ttTOWISxprUSfKFOxDuJrosRIhVjDhdRaZBNWeWShcIaUDGHcjMMCjepAmEZAIRMtnbHWAFjvqbjajWPushIEcSgaPCSuDJmaBzrtZJRxtoYxlWHLD"
        "YORebvqZLGdzpwYtkmIdYHKEMRCYLvOuEplGJVTLXmRCfXSgWbDColmEaNFQXmoOLHjuyjzFrhMbsfvKkewcjGhPJspoGwFQTwpsPSriKXHNlRXRrF"
        "kumflcMIpcZisrkvWmwQlIfRFtgIkdJWSvcDzpUINBTQxxQYFMJoEwmoWBzBTOnkqUtpbMclpiUXiDEoXVafcmJPqaFsOFmxXbZvroMsJreGyLrlhW"
        "BGLNDtYvwcBUgDVkbcLUuTRuHQgmroeDWgZTvNZjkyawQIVvRiuPAJBMGGgBjBnyYduvnulRqUbTxJgjSaeliZYLToTGUlMbjRfXdTXFtoLMYOKzfm"
        "hltNlYXYIFbnDZwkVRWdyVnblERSCILAhViByvZsOjzoJuHEpInOBVbcLbBFjLflcabLBWsbfLnrxlsMgGLjIIVlzfVvOkpADzQczfxPpgzUlQpDiB"
        "MfYDHgPNdgEVpLRzYJLJweJvlQuvwVLtVDiNhUAeKsLwvlmHCKozPFqlWoRVQlPwjdHrMlTcZkyvNIxJzpmSzkJaqROjQBFHWAusCBnfdvnSBcjgGr";
        ov.bytecount.start = "3455";
        ov.date.start = "Tue, 13 Jun 2008 00:00:00";
        ov.number.start = "200";
        ov.messageid.start = "<200>";
        str += nntp::make_overview(ov);


        buff.clear();
        buff.append(str);
        buff.set_content_length(str.size());
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::overview);
        buff.set_status(newsflash::buffer::status::success);
        cmd->ReceiveDataBuffer(buff);

        u.Complete(*cmd, actions);
        auto act = std::move(actions[0]);
        act->perform();
        BOOST_REQUIRE(!act->has_exception());
        actions.clear();
        u.Complete(*act, actions);
        act = std::move(actions[0]);
        act->perform();
        BOOST_REQUIRE(!act->has_exception());
        actions.clear();
        u.Complete(*act, actions);
        u.Commit();
    }

    // verify contents
    {
        using catalog = newsflash::catalog<newsflash::filemap>;
        using article = newsflash::article<newsflash::filemap>;
        using arraydb = newsflash::idlist<newsflash::filemap>;

        catalog db;
        db.open("alt.binaries.test/vol000000000000000.dat");
        BOOST_REQUIRE(db.size() == 6);

        arraydb idb;
        idb.open("alt.binaries.test/alt.binaries.test.idb");

        catalog::offset_t off(0);

        article a = db.load(off);
        BOOST_REQUIRE(a.subject() == "Metallica - Enter Sandman yEnc (01/10).mp3");
        BOOST_REQUIRE(a.author() == "ensi@gmail.com");
        BOOST_REQUIRE(a.test(newsflash::fileflag::binary));
        BOOST_REQUIRE(a.test(newsflash::fileflag::broken));
        BOOST_REQUIRE(a.bytes() == 1024 + 568 + 100);
        BOOST_REQUIRE(a.num_parts_total() == 10);
        BOOST_REQUIRE(a.num_parts_avail() == 3);
        BOOST_REQUIRE(idb[a.idbkey() + 1] + a.number() == 5);
        BOOST_REQUIRE(idb[a.idbkey() + 2] + a.number() == 2);
        BOOST_REQUIRE(idb[a.idbkey() + 3] + a.number() == 7);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == ".net and COM interoperability");
        BOOST_REQUIRE(a.author() == "foo@gmail.com");
        BOOST_REQUIRE(a.test(newsflash::fileflag::binary) == false);
        BOOST_REQUIRE(a.test(newsflash::fileflag::broken) == false);
        BOOST_REQUIRE(a.bytes() == 512);
        BOOST_REQUIRE(a.num_parts_total() == 0);
        BOOST_REQUIRE(a.num_parts_avail() == 0);
        BOOST_REQUIRE(a.number() == 454);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == "#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (1/4)");
        BOOST_REQUIRE(a.author() == "ano@anonymous.yy (knetje)");
        BOOST_REQUIRE(a.test(newsflash::fileflag::binary));
        BOOST_REQUIRE(a.test(newsflash::fileflag::broken) == true);
        BOOST_REQUIRE(a.num_parts_total() == 4);
        BOOST_REQUIRE(a.num_parts_avail() == 3);
        BOOST_REQUIRE(a.bytes() == 3 * 2048);
        BOOST_REQUIRE(idb[a.idbkey() + 1] + a.number() == 30);
        BOOST_REQUIRE(idb[a.idbkey() + 2] + a.number() == 40);
        BOOST_REQUIRE(idb[a.idbkey() + 3] + a.number() == 50);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (102).jpg (1/1)");
        BOOST_REQUIRE(a.author()  == "foo@acme.com");
        BOOST_REQUIRE(a.is_broken() == false);
        BOOST_REQUIRE(a.num_parts_total() == 1);
        BOOST_REQUIRE(a.num_parts_avail() == 1);
        BOOST_REQUIRE(idb[a.idbkey() + 1] + a.number() == 100);


        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (103).jpg (1/1)");
        BOOST_REQUIRE(a.author()  == "foo@acme.com");
        BOOST_REQUIRE(a.is_broken() == false);
        BOOST_REQUIRE(a.num_parts_total() == 1);
        BOOST_REQUIRE(a.num_parts_avail() == 1);
        BOOST_REQUIRE(idb[a.idbkey() + 1] + a.number() == 101);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (104).jpg (2/2)");
        BOOST_REQUIRE(a.author()  == "foo@acme.com");
        BOOST_REQUIRE(a.num_parts_total() == 2);
        BOOST_REQUIRE(a.num_parts_avail() == 1);
        BOOST_REQUIRE(a.is_broken());
        BOOST_REQUIRE(idb[a.idbkey() + 2] + a.number() == 102);

    }

    // update with more data
    {
        newsflash::Update u("", "alt.binaries.test");

        auto cmd = u.CreateCommands();

        newsflash::buffer buff(1024 * 5);
        buff.append("211 49999 1 500000 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 49999 1 500000 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->ReceiveDataBuffer(buff);

        u.Complete(*cmd, actions);

        cmd = u.CreateCommands();

        std::string str;
        nntp::overview ov  = {};
        ov.author.start    = "ano@anonymous.yy (knetje)";
        ov.subject.start   = "#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (4/4)";
        ov.bytecount.start = "100";
        ov.date.start      = "tue, 14 Jun 2008 00:00:00";
        ov.number.start    = "235242";
        ov.messageid.start = "<6>";
        str += nntp::make_overview(ov);

        ov.author.start    = "foo@acme.com";
        ov.subject.start   = "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (104).jpg (1/2)";
        ov.bytecount.start = "150";
        ov.date.start      = "Tue, 13 Jun 2008 00:00:00";
        ov.number.start    = "103";
        ov.messageid.start = "<103>";
        str += nntp::make_overview(ov);

        // uuencoded multipart binary.
        ov.author.start    = "ensi@gmail.com";
        ov.subject.start   = "Kelli Smith (v2) IMG_0108.JPEG (3/3)";
        ov.bytecount.start = "1024";
        ov.date.start      = "Tue, 13 May 2006 00:00:00";
        ov.number.start    = "303";
        ov.messageid.start = "<300>";
        str += nntp::make_overview(ov);

        ov.author.start    = "ensi@gmail.com";
        ov.subject.start   = "Kelli Smith (v2) IMG_0108.JPEG (1/3)";
        ov.bytecount.start = "1024";
        ov.date.start      = "Tue, 13 May 2006 00:00:00";
        ov.number.start    = "301";
        ov.messageid.start = "<100>";
        str += nntp::make_overview(ov);

        ov.author.start    = "ensi@gmail.com";
        ov.subject.start   = "Kelli Smith (v2) IMG_0108.JPEG (2/3)";
        ov.bytecount.start = "1024";
        ov.date.start      = "Tue, 13 May 2006 00:00:00";
        ov.number.start    = "302";
        ov.messageid.start = "<200>";
        str += nntp::make_overview(ov);

        // duplicates
        ov.author.start    = "whatever@foo.com";
        ov.subject.start   = "summer 2015 - File 63 of 73 - 1000034.jpg (1/1)";
        ov.bytecount.start = "2430";
        ov.date.start      = "Tue, 14 May 2015 00:00:00";
        ov.number.start    = "402";
        ov.messageid.start = "<402>";
        str += nntp::make_overview(ov);

        ov.author.start    = "whatever@foo.com";
        ov.subject.start   = "summer 2015 - File 63 of 73 - 1000034.jpg (1/1)";
        ov.bytecount.start = "2430";
        ov.date.start      = "Tue, 14 May 2015 00:00:00";
        ov.number.start    = "403";
        ov.messageid.start = "<403>";
        str += nntp::make_overview(ov);



        buff.clear();
        buff.append(str);
        buff.set_content_length(str.size());
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::overview);
        buff.set_status(newsflash::buffer::status::success);
        cmd->ReceiveDataBuffer(buff);

        u.Complete(*cmd, actions);
        auto act = std::move(actions[0]);
        act->perform();
        BOOST_REQUIRE(!act->has_exception());

        actions.clear();

        u.Complete(*act, actions);
        act = std::move(actions[0]);
        act->perform();
        BOOST_REQUIRE(!act->has_exception());
        actions.clear();
        u.Complete(*act, actions);
        u.Commit();

    }

    // verify contents
    {

        using catalog = newsflash::catalog<newsflash::filemap>;
        using article = newsflash::article<newsflash::filemap>;
        using arraydb = newsflash::idlist<newsflash::filemap>;

        catalog db;
        db.open("alt.binaries.test/vol000000000000000.dat");
        BOOST_REQUIRE(db.size() == 8);

        arraydb idb;
        idb.open("alt.binaries.test/alt.binaries.test.idb");

        catalog::offset_t off(0);
        article a = db.load(off);
        BOOST_REQUIRE(a.subject() == "Metallica - Enter Sandman yEnc (01/10).mp3");
        BOOST_REQUIRE(a.author() == "ensi@gmail.com");
        BOOST_REQUIRE(a.test(newsflash::fileflag::binary));
        BOOST_REQUIRE(a.test(newsflash::fileflag::broken));
        BOOST_REQUIRE(a.bytes() == 1024 + 568 + 100);
        BOOST_REQUIRE(a.num_parts_total() == 10);
        BOOST_REQUIRE(a.num_parts_avail() == 3);
        BOOST_REQUIRE(idb[a.idbkey() + 1] + a.number() == 5);
        BOOST_REQUIRE(idb[a.idbkey() + 2] + a.number() == 2);
        BOOST_REQUIRE(idb[a.idbkey() + 3] + a.number() == 7);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == ".net and COM interoperability");
        BOOST_REQUIRE(a.author() == "foo@gmail.com");
        BOOST_REQUIRE(a.test(newsflash::fileflag::binary) == false);
        BOOST_REQUIRE(a.test(newsflash::fileflag::broken) == false);
        BOOST_REQUIRE(a.bytes() == 512);
        BOOST_REQUIRE(a.num_parts_total() == 0);
        BOOST_REQUIRE(a.num_parts_avail() == 0);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == "#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (1/4)");
        BOOST_REQUIRE(a.author() == "ano@anonymous.yy (knetje)");
        BOOST_REQUIRE(a.test(newsflash::fileflag::binary));
        BOOST_REQUIRE(a.test(newsflash::fileflag::broken) == false);
        BOOST_REQUIRE(a.num_parts_total() == 4);
        BOOST_REQUIRE(a.num_parts_avail() == 4);
        BOOST_REQUIRE(a.bytes() == 3 * 2048 + 100);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (102).jpg (1/1)");
        BOOST_REQUIRE(a.author()  == "foo@acme.com");
        BOOST_REQUIRE(a.is_broken() == false);
        BOOST_REQUIRE(a.num_parts_total() == 1);
        BOOST_REQUIRE(a.num_parts_avail() == 1);
        BOOST_REQUIRE(idb[a.idbkey() + 1] + a.number() == 100);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (103).jpg (1/1)");
        BOOST_REQUIRE(a.author()  == "foo@acme.com");
        BOOST_REQUIRE(idb[a.idbkey() + 1] + a.number() == 101);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (104).jpg (2/2)");
        BOOST_REQUIRE(a.author()  == "foo@acme.com");
        BOOST_REQUIRE(a.num_parts_total() == 2);
        BOOST_REQUIRE(a.num_parts_avail() == 2);
        BOOST_REQUIRE(a.is_broken() == false);
        BOOST_REQUIRE(idb[a.idbkey() + 1] + a.number() == 103);
        BOOST_REQUIRE(idb[a.idbkey() + 2] + a.number() == 102);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == "Kelli Smith (v2) IMG_0108.JPEG (3/3)");
        BOOST_REQUIRE(a.author() == "ensi@gmail.com");
        BOOST_REQUIRE(a.num_parts_total() == 3);
        BOOST_REQUIRE(a.num_parts_avail() == 3);
        BOOST_REQUIRE(a.is_broken() == false);
        BOOST_REQUIRE(idb[a.idbkey() + 1] + a.number() == 301);
        BOOST_REQUIRE(idb[a.idbkey() + 2] + a.number() == 302);
        BOOST_REQUIRE(idb[a.idbkey() + 3] + a.number() == 303);

        off += a.size_on_disk();
        a = db.load(off);
        BOOST_REQUIRE(a.subject() == "summer 2015 - File 63 of 73 - 1000034.jpg (1/1)");
        BOOST_REQUIRE(a.author() == "whatever@foo.com");
        BOOST_REQUIRE(a.num_parts_total() == 1);
        BOOST_REQUIRE(a.num_parts_avail() == 1);
        BOOST_REQUIRE(a.is_broken() == false);
        BOOST_REQUIRE(a.bytes() == 2430);
    }
    delete_file("alt.binaries.test/vol000000000000000.dat");
    delete_file("alt.binaries.test/alt.binaries.test.nfo");
    delete_file("alt.binaries.test/alt.binaries.test.idb");
}


void unit_test_index()
{
    std::vector<std::unique_ptr<newsflash::action>> actions;

    fs::createpath("alt.binaries.test");
    delete_file("alt.binaries.test/vol000000000025676.dat");
    delete_file("alt.binaries.test/alt.binaries.test.nfo");
    delete_file("alt.binaries.test/alt.binaries.test.idb");

    // create data
    {
        newsflash::Update u("", "alt.binaries.test");

        auto cmd = u.CreateCommands();
        newsflash::buffer buff(1024);
        buff.append("211 10 1 5 alt.binaries.test group selected\r\n");
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        buff.set_content_start(0),
        buff.set_content_length(std::strlen("211 10 1 5 alt.binaries.test group selected\r\n"));
        cmd->ReceiveDataBuffer(buff);

        u.Complete(*cmd, actions);

        cmd = u.CreateCommands();

        auto data = read_file_buffer("test_data/xover.nntp.txt");
        //auto data = read_file_buffer("/tmp/nntp.txt");
        data.set_content_type(newsflash::buffer::type::overview);
        cmd->ReceiveDataBuffer(data);

        u.Complete(*cmd, actions);

        auto a = std::move(actions[0]);
        a->perform();
        BOOST_REQUIRE(!a->has_exception());
        actions.clear();

        u.Complete(*a, actions);

        a = std::move(actions[0]);
        a->perform();
        BOOST_REQUIRE(!a->has_exception());
        actions.clear();
        u.Complete(*a, actions);
        u.Commit();
    }

    // load the data
    {
        using catalog = newsflash::catalog<newsflash::filemap>;
        using index   = newsflash::index<newsflash::filemap>;
        using article = newsflash::article<newsflash::filemap>;

        catalog dbs[1];
        dbs[0].open("alt.binaries.test/vol000000000025676.dat");
        //dbs[1].open("alt.binaries.test/vol000000000033653.dat");

        index idx;
        idx.on_load = [&](std::size_t key, std::size_t idx) {
            return dbs[key].load(catalog::index_t{idx});
        };
        idx.on_filter = [](const article& a) {
            return true;
        };

        for (int i=0; i<1; ++i)
        {
            auto& db = dbs[i];
            auto beg = db.begin();
            auto end = db.end();
            for (; beg != end; ++beg)
            {
                const auto& article = *beg;
                idx.insert(article, i, article.index());
            }
        }

        {
            idx.sort(index::sorting::sort_by_subject, index::sortdir::ascending);

            for (std::size_t i=0; i<idx.size(); ++i)
            {
                const auto& a = idx[i];
                std::cout << a.subject() << std::endl;
            }
        }
    }

    delete_file("alt.binaries.test/vol000000000025676.dat");
    delete_file("alt.binaries.test/alt.binaries.test.nfo");
    delete_file("alt.binaries.test/alt.binaries.test.idb");

}


int test_main(int, char* [])
{
    unit_test_ranges();
    unit_test_data();
    unit_test_index();
    return 0;
}
