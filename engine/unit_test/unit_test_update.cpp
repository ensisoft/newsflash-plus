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
#include "../update.h"
#include "../cmdlist.h"
#include "../session.h"
#include "../buffer.h"
#include "../nntp.h"
#include "../filesys.h"
#include "../filemap.h"
#include "../catalog.h"
#include "../index.h"
#include "../array.h"
#include "unit_test_common.h"

void unit_test_ranges()
{
    delete_file("alt.binaries.test.nfo");

    std::string str;
    newsflash::session session;
    session.on_send = [&](const std::string& c) {
        str += c;
    };
    session.enable_pipelining(true);

    std::vector<std::unique_ptr<newsflash::action>> actions;

    {
        newsflash::update u("", "alt.binaries.test");

        auto cmd = u.create_commands();

        newsflash::buffer buff(1024);
        buff.append("211 10 1 1500 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 10 1 1500 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->receive_data_buffer(buff);

        u.complete(*cmd, actions);

        str.clear();

        cmd = u.create_commands();
        cmd->submit_data_commands(session);        
        session.send_next();
        BOOST_REQUIRE(str == "XOVER 501-1500\r\nXOVER 1-500\r\n");

    }

    std::ofstream out("alt.binaries.test.nfo", std::ios::out | std::ios::binary | std::ios::trunc);
    //out << "1045" << std::endl;
    //out << "2048" << std::endl;
    std::uint64_t val;
    val = 1045;
    out.write((const char*)&val, sizeof(val));
    val = 2048;
    out.write((const char*)&val, sizeof(val));
    out.close();

    // newer items
    {
        newsflash::update u("", "alt.binaries.test");

        auto cmd = u.create_commands();

        newsflash::buffer buff(1024);
        buff.append("211 3500 1045 3052 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 3500 1045 3052 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->receive_data_buffer(buff);

        u.complete(*cmd, actions);

        str.clear();

        cmd = u.create_commands();
        cmd->submit_data_commands(session);
        session.send_next();
        BOOST_REQUIRE(str == "XOVER 2049-3048\r\nXOVER 3049-3052\r\n");
        
    }


    // older items 
    {
        newsflash::update u("", "alt.binaries.test");

        auto cmd = u.create_commands();

        newsflash::buffer buff(1024);
        buff.append("211 3500 348 2048 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 3500 348 2048 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->receive_data_buffer(buff);

        u.complete(*cmd, actions);
        str.clear();

        cmd = u.create_commands();
        cmd->submit_data_commands(session);
        session.send_next();
        BOOST_REQUIRE(str == "XOVER 348-1044\r\n");
    }

    // no new items
    {
        newsflash::update u("", "alt.binaries.test");

        auto cmd = u.create_commands();

        newsflash::buffer buff(1024);
        buff.append("211 3500 1045 2048 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 3500 1045 2048 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->receive_data_buffer(buff);

        u.complete(*cmd, actions);
        str.clear();

        BOOST_REQUIRE(u.has_commands() == false);
    }

    delete_file("alt.binaries.test.nfo");    
}

void unit_test_data()
{
    std::vector<std::unique_ptr<newsflash::action>> actions;

    fs::createpath("alt.binaries.test");
    delete_file("alt.binaries.test/vol0.dat");
    delete_file("alt.binaries.test.nfo");    
    delete_file("alt.binaries.test.idb");

    {

        newsflash::update u("", "alt.binaries.test");

        auto cmd = u.create_commands();

        newsflash::buffer buff(1024 * 10);
        buff.append("211 49999 1 500000 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 49999 1 500000 alt.binaries.test group selected\r\n"));
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->receive_data_buffer(buff);

        u.complete(*cmd, actions);

        cmd = u.create_commands();

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
        ov.subject.start   = "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (104).jpg (1/4)";
        ov.bytecount.start = "100";
        ov.date.start      = "Tue, 13 Jun 2008 00:00:00";
        ov.number.start    = "102";
        ov.messageid.start = "<102>";
        str += nntp::make_overview(ov);        

        // these items are not valid and should be discarded.
        ov.author.start    = "foo@acme.com";
        ov.subject.start   = "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (104).jpg (7/4)";
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
        cmd->receive_data_buffer(buff);

        u.complete(*cmd, actions);
        auto a = std::move(actions[0]);
        a->perform();
        BOOST_REQUIRE(!a->has_exception());
        actions.clear();
        u.complete(*a, actions);
        a = std::move(actions[0]);
        a->perform();
        BOOST_REQUIRE(!a->has_exception());
        actions.clear();
        u.complete(*a, actions);
        u.commit();

        using catalog = newsflash::catalog<newsflash::filemap>;
        using arraydb = newsflash::array<std::int16_t, newsflash::filemap>;

        catalog db;
        db.open("alt.binaries.test/vol0.dat");

        arraydb idb;
        idb.open("alt.binaries.test.idb");

        BOOST_REQUIRE(db.article_count() == 6);

        auto article = db.lookup(catalog::offset_t(0));
        BOOST_REQUIRE(article.subject == "Metallica - Enter Sandman yEnc (01/10).mp3");
        BOOST_REQUIRE(article.author == "ensi@gmail.com");
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::binary));
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::broken));
        BOOST_REQUIRE(article.bytes == 1024 + 568 + 100);
        BOOST_REQUIRE(article.partmax == 10);
        BOOST_REQUIRE(article.partno == 3);

        BOOST_REQUIRE(idb[article.idb + 1] + article.number == 5);
        BOOST_REQUIRE(idb[article.idb + 2] + article.number == 2);
        BOOST_REQUIRE(idb[article.idb + 3] + article.number == 7);

        article = db.lookup(catalog::offset_t{article.next()});
        BOOST_REQUIRE(article.subject == ".net and COM interoperability");
        BOOST_REQUIRE(article.author == "foo@gmail.com");
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::binary) == false);
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::broken) == false);
        BOOST_REQUIRE(article.bytes == 512);
        BOOST_REQUIRE(article.partmax == 0);
        BOOST_REQUIRE(article.partno == 0);


        article = db.lookup(catalog::offset_t{article.next()});
        BOOST_REQUIRE(article.subject == "#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (1/4)");
        BOOST_REQUIRE(article.author == "ano@anonymous.yy (knetje)");
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::binary));
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::broken) == true);
        BOOST_REQUIRE(article.partmax == 4);
        BOOST_REQUIRE(article.partno == 3);
        BOOST_REQUIRE(article.bytes == 3 * 2048);

        BOOST_REQUIRE(idb[article.idb + 1] + article.number == 30);
        BOOST_REQUIRE(idb[article.idb + 2] + article.number == 40);        
        BOOST_REQUIRE(idb[article.idb + 3] + article.number == 50);                

        article = db.lookup(catalog::offset_t{article.next()});
        BOOST_REQUIRE(article.subject == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (102).jpg (1/1)");
        BOOST_REQUIRE(article.author  == "foo@acme.com");
        BOOST_REQUIRE(idb[article.idb + 1] + article.number == 100);

        article = db.lookup(catalog::offset_t{article.next()});
        BOOST_REQUIRE(article.subject == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (103).jpg (1/1)");
        BOOST_REQUIRE(article.author  == "foo@acme.com");
        BOOST_REQUIRE(idb[article.idb + 1] + article.number == 101);        

        article = db.lookup(catalog::offset_t{article.next()});
        BOOST_REQUIRE(article.subject == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (104).jpg (1/4)");
        BOOST_REQUIRE(article.author  == "foo@acme.com");
        BOOST_REQUIRE(idb[article.idb + 1] + article.number == 102);                

    }

    // update and then verify the contents.
    {
        newsflash::update u("", "alt.binaries.test");

        auto cmd = u.create_commands();

        newsflash::buffer buff(1024);
        buff.append("211 49999 1 500000 alt.binaries.test group selected\r\n");
        buff.set_content_length(std::strlen("211 49999 1 500000 alt.binaries.test group selected\r\n"));        
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        cmd->receive_data_buffer(buff);

        u.complete(*cmd, actions);

        cmd = u.create_commands();

        std::string str;
        nntp::overview ov  = {};
        ov.author.start    = "ano@anonymous.yy (knetje)";
        ov.subject.start   = "#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (4/4)";
        ov.bytecount.start = "100";
        ov.date.start      = "tue, 14 Jun 2008 00:00:00";
        ov.number.start    = "235242";
        ov.messageid.start = "<6>";
        str += nntp::make_overview(ov);

        buff.clear();
        buff.append(str);
        buff.set_content_length(str.size());
        buff.set_content_start(0);
        buff.set_content_type(newsflash::buffer::type::overview);
        buff.set_status(newsflash::buffer::status::success);
        cmd->receive_data_buffer(buff);

        u.complete(*cmd, actions);
        auto a = std::move(actions[0]);
        a->perform();
        BOOST_REQUIRE(!a->has_exception());

        actions.clear();

        u.complete(*a, actions);
        a = std::move(actions[0]);
        a->perform();
        BOOST_REQUIRE(!a->has_exception());
        actions.clear();
        u.complete(*a, actions);
        u.commit();

        using catalog = newsflash::catalog<newsflash::filemap>;
        catalog db;
        db.open("alt.binaries.test/vol0.dat");
        BOOST_REQUIRE(db.article_count() == 6);

        auto article = db.lookup(catalog::offset_t(0));
        BOOST_REQUIRE(article.subject == "Metallica - Enter Sandman yEnc (01/10).mp3");
        BOOST_REQUIRE(article.author == "ensi@gmail.com");
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::binary));
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::broken));
        BOOST_REQUIRE(article.bytes == 1024 + 568 + 100);
        BOOST_REQUIRE(article.partmax == 10);
        BOOST_REQUIRE(article.partno == 3);

        article = db.lookup(catalog::offset_t{article.next()});
        BOOST_REQUIRE(article.subject == ".net and COM interoperability");
        BOOST_REQUIRE(article.author == "foo@gmail.com");
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::binary) == false);
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::broken) == false);
        BOOST_REQUIRE(article.bytes == 512);
        BOOST_REQUIRE(article.partmax == 0);
        BOOST_REQUIRE(article.partno == 0);

        article = db.lookup(catalog::offset_t{article.next()});
        BOOST_REQUIRE(article.subject == "#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (1/4)");
        BOOST_REQUIRE(article.author == "ano@anonymous.yy (knetje)");
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::binary));
        BOOST_REQUIRE(article.bits.test(newsflash::article::flags::broken) == false);
        BOOST_REQUIRE(article.partmax == 4);
        BOOST_REQUIRE(article.partno == 4);        
        BOOST_REQUIRE(article.bytes == 3 * 2048 + 100);

        article = db.lookup(catalog::offset_t{article.next()});
        BOOST_REQUIRE(article.subject == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (102).jpg (1/1)");
        BOOST_REQUIRE(article.author  == "foo@acme.com");
        //BOOST_REQUIRE(idb[article.idb + 1] + article.number == 100);

        article = db.lookup(catalog::offset_t{article.next()});
        BOOST_REQUIRE(article.subject == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (103).jpg (1/1)");
        BOOST_REQUIRE(article.author  == "foo@acme.com");
        //BOOST_REQUIRE(idb[article.idb + 1] + article.number == 101);        

        article = db.lookup(catalog::offset_t{article.next()});
        BOOST_REQUIRE(article.subject == "girls flirting with is neat GiBBA files Soft I love you to BF-Vol3 (104).jpg (1/4)");
        BOOST_REQUIRE(article.author  == "foo@acme.com");
        //BOOST_REQUIRE(idb[article.idb + 1] + article.number == 102);                

    }
}


void unit_test_index()
{
    std::vector<std::unique_ptr<newsflash::action>> actions;

    fs::createpath("alt.binaries.test");
    delete_file("alt.binaries.test/vol33653.dat");
    delete_file("alt.binaries.test/vol33654.dat");
    delete_file("alt.binaries.test.nfo");
    delete_file("alt.binaries.test.idb");

    // create data
    {
        newsflash::update u("", "alt.binaries.test");

        auto cmd = u.create_commands();
        newsflash::buffer buff(1024);
        buff.append("211 10 1 5 alt.binaries.test group selected\r\n");
        buff.set_content_type(newsflash::buffer::type::groupinfo);
        buff.set_content_start(0),
        buff.set_content_length(std::strlen("211 10 1 5 alt.binaries.test group selected\r\n"));
        cmd->receive_data_buffer(buff);

        u.complete(*cmd, actions);

        cmd = u.create_commands();

        //auto data = read_file_buffer("test_data/xover.nntp.txt");
        auto data = read_file_buffer("/tmp/nntp.txt");
        data.set_content_type(newsflash::buffer::type::overview);
        cmd->receive_data_buffer(data);

        u.complete(*cmd, actions);

        auto a = std::move(actions[0]);
        a->perform();
        BOOST_REQUIRE(!a->has_exception());
        actions.clear();

        u.complete(*a, actions);

        a = std::move(actions[0]);
        a->perform();
        BOOST_REQUIRE(!a->has_exception());
        actions.clear();
        u.complete(*a, actions);
        u.commit();
    }

    // load the data
    {
        using catalog = newsflash::catalog<newsflash::filemap>;

        catalog dbs[2];
        dbs[0].open("alt.binaries.test/vol33654.dat");
        dbs[1].open("alt.binaries.test/vol33653.dat");

        newsflash::index index;
        index.on_load = [&](std::size_t key, std::size_t idx) {
            return dbs[key].lookup(catalog::index_t{idx});
        };

        {
            auto& db = dbs[0];
            auto beg = db.begin();
            auto end = db.end();
            for (; beg != end; ++beg)
            {
                const auto& article = *beg;
                index.insert(article, 0, article.index);
            }
        }

        {
            auto& db = dbs[1];
            auto beg = db.begin();
            auto end = db.end();
            for (; beg != end; ++beg)
            {
                const auto& article = *beg;
                index.insert(article, 1, article.index);
            }
        }

        {
            index.sort(newsflash::index::sorting::sort_by_subject,
                newsflash::index::order::ascending);

            for (std::size_t i=0; i<index.size(); ++i)
            {
                const auto& a = index[i];
                std::cout << a.subject << std::endl;
            }
        }
    }

    //delete_file("alt.binaries.test/vol33653.dat");
    //delete_file("alt.binaries.test/vol33654.dat");    
    //delete_file("alt.binaries.test.nfo");    
}

int test_main(int, char* [])
{
    unit_test_ranges();
    unit_test_data();
    unit_test_index();

    return 0;
}