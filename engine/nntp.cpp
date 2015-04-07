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
#  include <boost/spirit/include/classic.hpp>
#  include <boost/functional/hash.hpp>
#  include <boost/regex.hpp>
#include <newsflash/warnpop.h>
#include <sstream>
#include <algorithm>
#include <cctype>
#if defined(LINUX_OS)
#  include <strings.h> // for strcasecmp
#endif
#include "nntp.h"

namespace {
    bool strip_leading_space(const char*& str, size_t& len)
    {
        while (*str == ' ')
        {
            if (--len == 0)
                return false;
            ++str;
        }
        return true;
    }

    struct overview_parser {
        const char* str;
        const size_t len;
        size_t pos;
    };

    bool parse_overview_field(overview_parser& parser, nntp::overview::field& field)
    {
        field.start = nullptr;
        field.len   = 0;

        size_t len = 0;
        const char* start = &parser.str[parser.pos];
        for (; parser.pos < parser.len; ++parser.pos)
        {
            if (parser.str[parser.pos] == '\t')
            {
                if (len > 0)
                {
                    field.start = start;
                    field.len   = len;
                }
                ++parser.pos;
                return true;
            }
            ++len;
        }
        return false;
    }

    bool strip_leading_crap(overview_parser& parser)
    {
        for (; parser.pos < parser.len; ++parser.pos)
        {
            const auto c = (unsigned)parser.str[parser.pos];
            if (c >= 0x22 || c == '\t')
                return true;
        }
        return false;

        //const unsigned char* start = static_cast<const unsigned char*>((void*)&parser.str[parser.pos]);
        // while (*start < 0x22 && *start != '\t')
        // {
        //     if (++parser.pos == parser.len)
        //         return false;
        //     ++start;        
        // }
        // return true;
    }

    // http://www.fileinfo.com/common.php

    // construct an ugly reg exp and try to see if the subject line matches.
    // try to keep the most common stuff at the front.
    // note that the regular expression is in *reverse* because it's matched in reverse

    // 001, 002, 003... files are HJSplit files
    // Container formats: AVI, WMV, MP4, OGG, OMG, MKV, MKA
    // Compression formats: RAR, ZIP, GZ, 7z
    // Image formats: BMP, GIF, JPG, JPEG, PNG
    // Audio formats: MP3, MP2, WAV
    // DNS: Nintendo DS
    // SFW: flash
    const char* REVERSE_FILE_EXTENSION_REGEX = 
        R"(rar\.|2rap\.|rap\.|ge?pj\.|iva\.|[234]pm\.|ge?pm\.|gnp\.|)" \
        R"(fig\.|fdp\.|ggo\.|vmw\.|vom\.|osi\.|nib\.|piz\.|vaw\.|amw\.|)" \
        R"(pmb\.|vfs\.|[0-9][0-9]r\.|ofn\.|euc\.|u3m\.|alf\.|bzn\.|mgo\.|)" \
        R"([0-9]{3}\.|xvid\.|exe\.|mr\.|a4m\.|bov\.|vkm\.|akm\.|eca\.|mar\.|calf\.|)" \
        R"([0-9]{1,3}z\.|fdm\.|tad\.|z7\.|caa\.|grn\.|cpm\.|fit\.)"\
        R"([0-9]{1,3}s\.|ffai\.|fws\.|sdn\.|st\.|3ca\.|rrs\.)";

class reverse_c_str_iterator :
  public std::iterator<std::bidirectional_iterator_tag, const char>
{
public:
    reverse_c_str_iterator() : ptr_(nullptr) {}
    reverse_c_str_iterator(const char* str) : ptr_(str) {}

    // postfix
    reverse_c_str_iterator operator++(int)
    {
        reverse_c_str_iterator it(*this);
        --ptr_;
        return it;
    }
    reverse_c_str_iterator& operator++()
    {
        --ptr_;
        return *this;
    }
    // postfix
    reverse_c_str_iterator operator--(int)
    {
        reverse_c_str_iterator it(*this);
        ++ptr_;
        return it;
    }
    reverse_c_str_iterator& operator--()
    {
        ++ptr_;
        return *this;
    }

    const char& operator*() const
    {
        assert(ptr_);
        return *ptr_;
    }
    
    const char* as_ptr() const
    {
        return ptr_;
    }
    bool operator != (const reverse_c_str_iterator& rhs) const
    {
        return ptr_ != rhs.ptr_;
    }
    bool operator == (const reverse_c_str_iterator& rhs) const
    {
        return ptr_ == rhs.ptr_;
    }
    
private:
    const char* ptr_;
};

const char* find_part_count(const char* subjectline, std::size_t len, std::size_t& i)
{
    // NOTE: the regex is in reverse because the search is from the tail
    // this matches "metallica - enter sandman.mp3 (01/50)"
     const static boost::regex ex("\\)[0-9]*/[0-9]*\\(");
    
     reverse_c_str_iterator itbeg(subjectline + len - 1);
     reverse_c_str_iterator itend(subjectline-1);

     boost::match_results<reverse_c_str_iterator> res;

     if (!regex_search(itbeg, itend, res, ex))
         return NULL;
    
     if (res[0].second == itend)
         return NULL;

     const char* end   = res[0].first.as_ptr();  // at the last matched character
     const char* start = res[0].second.as_ptr(); // at one before the start of the match
     
     ++start;
     ++end;
     // some people use this gay notation of prefixing their stuff with (xx/yy) notation
     // to indicate all the articles of their posting "batch". This has nothing to do with 
     // with the yEnc part count hack. So if the search returned a match at the beginning, ignore this.
     if (start == subjectline)
         return NULL;

     i = end - start;
     return start;
}

std::string make_string(const nntp::overview::field& f)
{
    if (!f.start) return "";
    if (!f.len)  return {f.start, std::strlen(f.start)};
    return {f.start, f.len};
}

} // namespace

namespace nntp
{

std::string make_overview(const overview& ov)
{
    std::stringstream ss;
    ss << make_string(ov.number)     << "\t";    
    ss << make_string(ov.subject)    << "\t";    
    ss << make_string(ov.author)     << "\t";
    ss << make_string(ov.date)       << "\t";
    ss << make_string(ov.messageid)  << "\t";
    ss << make_string(ov.references) << "\t";
    ss << make_string(ov.bytecount)  << "\t";
    ss << make_string(ov.linecount)  << "\t\r\n";
    return ss.str();
}

bool is_binary_post(const char* str, size_t len)
{
    // TODO: consider separating this functionality into two functions.
    // 1 to find whether a post is should be collapsed, i.e. is multipart post
    // 2 to find whether the post is a binary post
    // http://www.boost.org/libs/regex/doc/syntax_perl.html      
    const static boost::regex filename(REVERSE_FILE_EXTENSION_REGEX, boost::regbase::icase | boost::regbase::perl);
    const static boost::regex yenc("yEnc *\\([0-9]*/[0-9]*\\)");
    const static boost::regex part("\\([0-9]*/[0-9]*\\)|yEnc");

    const char* start = str + len - 1;
    const char* end   = nullptr;
    while (true)
    {
        reverse_c_str_iterator itbeg(start);
        reverse_c_str_iterator itend(str - 1);
        
        boost::match_results<reverse_c_str_iterator> res;

        if (!regex_search(itbeg, itend, res, filename))
        {
            // a desperate attempt....
            // if there's 'yEnc (xx/yy)' we just assume its a yEnc binary
            if (regex_search(str, str+len, yenc))
                return true;
            return false;
        }

        // first returns an iterator pointing to the
        // start of the sequence, which in this case is the end of
        // the filename extension
        end = res[0].first.as_ptr();
        // second returns an iterator pointing one past the end of the sequence,
        // which in this case is one before the start of the sequence
        start = res[0].second.as_ptr();

        // move start to the first character (.)
        ++start;
        // move end one past the last character in the extension
        ++end;

        // if the subject line starts with the filename extension
        // we assume this post in question is not a binary post
        // but something like ".pdf viewer for linux?"
        if (start == str)
            return false;
        
        // if filename is matched at the end of the subjectline
        // this is ok
        if (end == str + len)
            return true;

        --start;

        assert(end < str + len);        
        assert(start >= str);
        
        // if the filename ends with a space (very common), 
        // or with a double quote this is ok and the extension does not
        // have a space right before it then it is ok
        
        // if the filename ends with a quote this is ok.
        // (yEnc encoded posts should have a subject line like '"fooobar.mp3 (1/10)" yEnc'
        // another used notation is '(fooobar.jpeg')
        if (*end == '"' || *end == ')')
            return true;
        
        if (*start != ' ')
        {
            // a space after a filename this is swell. Probably some non-yEnc encoded
            // binary. For example 'anna-kournikova.JPEG - sweet ass
            if (*end == ' ')
                return true;

            // finally there's a case that falls through everything else.
            // Ie. we have a match like 'metallica - enter sandman.mp3Posted by keke'
            // see if the rest of the string contains either 'yEnc' or '(xx/yy)'. if it does then this is ok, otherwise fail.
            if (regex_search(start, str+len, part))
                return true;
        }
        else
        {
            if (regex_search(start, str+len, part))
                return true;
        }
    }

    return false;    
}

bool strcmp(const char* first,  std::size_t firstLen,
                const char* second, std::size_t secondLen)
{

    if (firstLen != secondLen)
        return false;

    const auto len = firstLen;
    bool beginPart = false;

    for (size_t i=0; i<len; ++i)
    {
        if (first[i] != second[i])
        {
            if (beginPart)
            {
                if (!std::isdigit(first[i]))
                    return false;
                continue;
            }
            return false;
        }
        if (beginPart) 
        {
            if (first[i] == ')') 
                beginPart = false;
        } 
        else beginPart = first[i] == '(';
    }

    return true;
    
} 


std::time_t timevalue(const nntp::date& date)
{
    enum { USE_SYSTEM_DAYLIGHT_SAVING_INFO = -1 };

    if (date.year == 0) return 0;

    time_t ret  = 0;
    struct tm t = {};
    t.tm_sec    = date.seconds;
    t.tm_min    = date.minutes;
    t.tm_hour   = date.hours;
    t.tm_mday   = date.day;
    t.tm_year   = date.year - 1900;
    t.tm_isdst  = USE_SYSTEM_DAYLIGHT_SAVING_INFO;

    // todo: can this depend on the locale where where
    // the date was generated??
    const char* arr[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    for (size_t i=0; i<sizeof(arr)/sizeof(char*); ++i)
    {
#if defined(LINUX_OS)
        if (strcasecmp(date.month.c_str(), arr[i]) == 0)
        {
            t.tm_mon = i;
            break;
        }
#elif defined(WINDOWS_OS)
        if (_stricmp(date.month.c_str(), arr[i]) == 0)
        {
            t.tm_mon = i;
            break;
        }
#endif
    }
    ret = mktime(&t);
    if (date.tzoffset)
    {
        // in order to make all times comparable we convert all times to GMT times
        // by reducing/adding the relevant time zone offset to the seconds.
        ret += ((date.tzoffset / 100) * -3600);
        ret += ((date.tzoffset % 100) * -60);
    }
    else if (!date.tz.empty())
    {
        // todo: translate the TZ name into a delta to GMT
    }
    return ret;
}

std::pair<bool, overview> parse_overview(const char* str, size_t len)
{
    overview ov;
    std::memset(&ov, 0, sizeof(ov));

    if (!strip_leading_space(str, len))
        return {false, ov};

    overview_parser parser {str, len, 0};

    bool ret = true;
    ret = ret & parse_overview_field(parser, ov.number);


    strip_leading_crap(parser);

    ret = ret & parse_overview_field(parser, ov.subject);
    ret = ret & parse_overview_field(parser, ov.author);
    ret = ret & parse_overview_field(parser, ov.date);
    ret = ret & parse_overview_field(parser, ov.messageid);
    ret = ret & parse_overview_field(parser, ov.references);
    ret = ret & parse_overview_field(parser, ov.bytecount);
    ret = ret & parse_overview_field(parser, ov.linecount);

    return {ret, ov};
}

std::pair<bool, group> parse_group_list_item(const char* str, size_t len)
{
    strip_leading_space(str, len);

    std::stringstream ss(std::string(str, len));

    group grp;
    ss >> std::skipws >> grp.name;
    ss >> std::skipws >> grp.last;
    ss >> std::skipws >> grp.first;
    ss >> std::skipws >> grp.posting;

    bool success = !ss.fail();

    return { success, grp};
}

std::pair<bool, group> parse_group(const char* str, size_t len)
{
    group grp;    
    try 
    {
        std::string estimate;
        nntp::scan_response({211}, str, len, estimate, grp.first, grp.last, grp.name);
    }
    catch (const std::exception& e)
    { 
        return {false, grp };
    }
    return {true, grp};
}

std::pair<bool, date> parse_date(const char* str, size_t len)
{
    nntp::date date {};

    using namespace boost::spirit::classic;

    // Thu, 26 Jul 2007 19:44:13 -0500
    auto ret = parse(str, str+len,
        (
         alpha_p >> alpha_p >> alpha_p >> ',' >>
         (uint_p[assign(date.day)]) >>
         ((repeat_p(3)[alpha_p])[assign(date.month)]) >>
         (uint_p[assign(date.year)]) >>
         (uint_p[assign(date.hours)]) >> ':' >>
         (uint_p[assign(date.minutes)]) >> ':' >>
         (uint_p[assign(date.seconds)]) >>
         !(int_p[assign(date.tzoffset)]) >>
         !(!ch_p('(') >> (*alpha_p)[assign(date.tz)] >> !ch_p(')'))
         ), ch_p(' '));
    if (ret.full)
        return {true, date};

    // 29 Jul 2007 11:25:26 GMT
    ret = parse(str, str+len,
        (
         (uint_p[assign(date.day)]) >>
         ((repeat_p(3)[alpha_p])[assign(date.month)]) >>
         (uint_p[assign(date.year)]) >>
         (uint_p[assign(date.hours)]) >> ':' >>
         (uint_p[assign(date.minutes)]) >> ':' >>
         (uint_p[assign(date.seconds)]) >>
         !(int_p[assign(date.tzoffset)])     >>
         !((*alpha_p)[assign(date.tz)])
        ), ch_p(' '));
    if (ret.full) 
        return {true, date};

    // Wednesday, 24 Oct 2008 11:58:50 -0800
    ret = parse(str, str+len,
        (
         (*alpha_p) >>  ',' >>
         (uint_p[assign(date.day)]) >>
         ((repeat_p(3)[alpha_p])[assign(date.month)]) >>
         (uint_p[assign(date.year)]) >>
         (uint_p[assign(date.hours)]) >> ':' >>
         (uint_p[assign(date.minutes)]) >> ':' >>
         (uint_p[assign(date.seconds)]) >>
         !(int_p[assign(date.tzoffset)]) >>
         !((*alpha_p)[assign(date.tz)])
         ), ch_p(' '));

    return {ret.full, date};

}

std::pair<bool, part> parse_part(const char* str, size_t len)
{
    nntp::part part {0};

    str = find_part_count(str, len, len);
    if (!str) 
        return {false, part};

    using namespace boost::spirit::classic;

    const auto& ret = parse(str, str+len,
        (                                   
         ch_p('(') >> uint_p[assign(part.numerator)] >> 
         ch_p('/') >> uint_p[assign(part.denominator)] >>
         ch_p(')')
        ));
    return {ret.full, part};
}

std::uint32_t hashvalue(const char* subjectline, size_t len)
{
    std::size_t seed = 0;

    bool escape = false;

    for (size_t i=0; i<len; ++i)
    {
        const unsigned char val = subjectline[i];
        if (val == '(')
            escape = true;
        else if (val == ')')
            escape = false;
        else
        {
            if (escape)
            {
                if (!std::isdigit(val))
                    boost::hash_combine(seed, val);
            }
            else
            {
                boost::hash_combine(seed, val);
            }
        }
    }
    return std::uint32_t(seed);
}

std::string find_filename(const char* str, size_t len)
{
    using namespace boost::spirit::classic;
    
    // see if it's an yEnc subjectline match.
    std::string yenc_name;
    const auto ret = parse(str, str + len,
        (*(anychar_p - '"') >> str_p("\"") >> (*(anychar_p - '"'))[assign(yenc_name)] >> str_p("\" yEnc")));
    if (ret.hit)
        return yenc_name;

    const static boost::regex regex(REVERSE_FILE_EXTENSION_REGEX, boost::regbase::icase | boost::regbase::perl);

    reverse_c_str_iterator itbeg(str + len - 1);
    reverse_c_str_iterator itend(str - 1);

    boost::match_results<reverse_c_str_iterator> res;
    
    if (!regex_search(itbeg, itend, res, regex))
        return "";

    // first returns an iterator pointing to the 
    // start of the sequence, which in this case is the end of the filename
    // extension.
    const char* end = res[0].first.as_ptr(); 
    // second returns an iterator pointing one past the end of the sequence
    // which in this case is one before the the start of the filename extension
    const char* start = res[0].second.as_ptr();

    if (start < str)
        ++start;

    // seek the dot, its part of the regex so it must be found before start of str    
    const char* dot = end;
    while (*dot != '.')
        --dot;

    char start_marker = 0;
    // look at the first character after the filename extension and see if it's a special
    // character such as ", ], )
    if (++end < str + len)
    {
        if (*end == '"')
            start_marker = '"';
        else if (*end == ')')
            start_marker = '(';
        else if (*end == ']')
            start_marker = '[';
        else if (*end == '#')
            start_marker = '#';
    }

    if (start_marker)
    {
        while (*start != start_marker && start > str)
            --start;

        // if we hit the marker exclude it from the name
        if (*start == start_marker)
            ++start;

        // trim whitespace
        while (std::isspace(*start) && start < dot)
            ++start;
    }
    else
    {
        // seek untill first non space or dash alphanumeric character is found
        while ((std::isalnum(*start) || *start == '-' || *start == '_')
            && start > str)
            --start;

        // trim leading non-alpha
        while (!std::isalnum(*start) && start < dot)
            ++start;            
    }

    assert(start <= dot && dot < end);

    if (start - dot == 0)
        return "";

    return std::string(start, end-start);
}

std::size_t find_response(const void* buff, std::size_t size)
{
    // end of response is indicated by \r\n
    if (size < 2)
        return 0;

    const char* str = static_cast<const char*>(buff);            

    for (std::size_t len=1; len<size; ++len)
    {
        if (str[len] == '\n' && str[len-1] == '\r')
            return len+1;
    }
    return 0;
}

std::size_t find_body(const void* buff, std::size_t size)
{
    // this is referred to as "multi-line data block" in rfc3977
    // each block consists of a sequence of zero or more "lines",
    // each being a stream of octets ending with CLRF pair.
    // finally lines are followed by a terminating line consisting of
    // a single "terminating octet" (.) followed by a CRLF.
    //
    // thus for non-empty cases one must look for CRLF . CRLF.
    // note that you can't simply look for . CRLF cause for example
    // human readable text body often ends with a . CRLF. so for the
    // non-empty case we *must* look for CRLF . CRLF. Dot doubling
    // in user text makes sure that a single dot will never begin
    // a line of body text.
    //
    //     response-line CRLF
    //     line CRLF
    //     line CRLF
    //        ...
    //     . CRLF
    //
    // and in case of empty body case we must look for . CRLF only.
    //
    //     response line CRLF 
    //     . CRLF

    const char* str = static_cast<const char*>(buff);    
    if (size == 3)
    {
        if (str[0] == '.' && str[1] == '\r' && str[2] == '\n')
            return 3;
    }
    else if (size < 5)
        return 0;

    for (std::size_t len=4; len<size; ++len)
    {
        if ((str[len-0] == '\n' && str[len-1] == '\r' && str[len-2] == '.') && 
            (str[len-3] == '\n' && str[len-4] == '\r'))
            return len+1;
    }
    return 0;
}

} // nntp
