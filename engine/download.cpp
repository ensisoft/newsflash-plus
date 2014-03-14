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

#include <boost/regex.hpp>
#include <boost/crc.hpp>
#include "nntp/linebuffer.h"
#include "download.h"
#include "content.h"
#include "buffer.h"


namespace {
    enum class encoding {
        yenc_single,
        yenc_multi,
        uuencode_single,
        uuencode_multi,
        unknown
    };

    encoding identify_encoding(const char* line, std::size_t len)
    {
        static const boost::regex yenc_multi("^=ybegin +part=[0-9]+( +total=[0-9]+)?( +line=[0-9]+)?( +size=[0-9]+)?( +line=[0-9]+)? +name=.*", 
            boost::regbase::icase | boost::regbase::perl);

        static const boost::regex yenc_single("^=ybegin +line=[0-9]+ +size=[0-9]+ +name=.*", 
            boost::regbase::icase | boost::regbase::perl);

        static const boost::regex uuencode("begin +[0-9]{3} +.*", 
            boost::regbase::icase | boost::regbase::perl);        

        const auto* beg = line;
        const auto* end = line + len;
        try
        {
            if (boost::regex_search(beg, end, yenc_multi))
                return encoding::yenc_multi;

            if (boost::regex_search(beg, end, yenc_single))
                return encoding::yenc_single;

            if (boost::regex_search(beg, end, uuencode))
                return encoding::uuencode_single;

            // lastly a final test, multipart uuencoded binary (typically a larger image file)
            // uuencode specification says that the length of every full line
            // is 60 and every line is prefixed with the uuencoded length
            if (*beg == 'M' && (len == 61 || len == 62 || len == 63))
                return encoding::uuencode_multi;
        }
        catch (const std::exception&)
        {}

        return encoding::unknown;
    }

} // namespace


namespace newsflash
{


download::download(std::string folder, std::string name) : folder_(std::move(folder)), name_(std::move(name))
{}

void download::prepare()
{}

void download::receive(const buffer& buff, std::size_t id)
{
    const nntp::linebuffer lines((const char*)buffer_payload(buff), 
        buffer_payload_size(buff));

    auto beg = lines.begin();
    auto end = lines.end();

    while (beg != end)
    {
        const auto& line = *beg;
        const auto enc = identify_encoding(line.start, line.length);
        if (enc != encoding::yenc_multi)
        {

        }
    }
}

void download::cancel()
{}

void download::flush()
{}

void download::finalize()
{}



} // newsflash

