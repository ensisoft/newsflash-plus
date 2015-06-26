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
#include <newsflash/warnpush.h>
#  include <boost/regex.hpp>
#include <newsflash/warnpop.h>
#include "encoding.h"

namespace newsflash
{

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

} // newsflash
