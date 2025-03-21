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
#  include <boost/spirit/include/classic.hpp>
#  include <boost/functional/hash.hpp>
#  include <boost/regex.hpp>
#include "newsflash/warnpop.h"

#include <sstream>
#include <algorithm>
#include <stack>
#include <cctype>

#ifndef BOOST_HAS_THREADS
#  error Boost is not in thread safe mode.
#endif

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
        R"([0-9]{1,3}s\.|ffai\.|fws\.|sdn\.|st\.|3ca\.|rrs\.)" \
        R"(v4m\.|vlf\.|mhc\.)";

    // part count notation indicates the segment of the file when the file is split
    // to multiple NNTP posts. Usual notation is (xx/yy) but sometimes [xx/yy] is used
    // also note that sometimes subject lines contain both. In this case the [xx/yy] is considered
    // to indicate the file number in the batch of files.
    // so [xx/yy] is not considered as part count unless (xx/yy) is not found.
    const char* find_part_count(const char* subjectline, std::size_t len, std::size_t& i)
    {
        // NOTE: the regex is in reverse because the search is from the tail
        // this matches "metallica - enter sandman.mp3 (01/50)"
         const static boost::regex ex("(\\]|\\))[0-9]*/[0-9]*(\\(|\\[)");

         nntp::reverse_c_str_iterator itbeg(subjectline + len - 1);
         nntp::reverse_c_str_iterator itend(subjectline - 1);

         boost::match_results<nntp::reverse_c_str_iterator> res;

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

    inline void default_boost_hash(size_t& seed, size_t value)
    {
        boost::hash_combine(seed, value);
    }

    nntp::hash_combine_function hash_combine = &default_boost_hash;

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

bool strcmp(const char* first,  std::size_t firstLen, const char* second, std::size_t secondLen)
{
    std::size_t skip_first  = 0;
    std::size_t skip_second = 0;

    const auto* p1 = find_part_count(first, firstLen, skip_first);
    if (!p1)
    {
        if (firstLen != secondLen)
            return false;

        for (std::size_t i=0; i<firstLen; ++i)
            if (first[i] != second[i]) return false;
    }
    else
    {
        const auto* p2 = find_part_count(second, secondLen, skip_second);
        if (!p2)
            return false;

        // the part count needs to begin at the same position
        if ((p1 - first) != (p2 - second))
            return false;

        assert(p1 >= first);
        assert(p2 >= second);

        std::size_t num = p1 - first;
        std::size_t i = 0;
        std::size_t j = 0;

        // compare the strings untill the part count.
        for (std::size_t x=0; x<num; ++x, ++i, ++j)
            if (first[i] != second[j]) return false;

        i += skip_first;
        j += skip_second;

        // if the reminder after the part count is not the same
        // length it's not a match.
        if (firstLen - i != secondLen - j)
            return false;

        num = firstLen - i;

        for (std::size_t x=0; x<num;  ++x, ++i, ++j)
            if (first[i] != second[j]) return false;
    }

    return true;

}


std::time_t timevalue(const nntp::date& date)
{
    enum { USE_SYSTEM_DAYLIGHT_SAVING_INFO = -1 };

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
    // if the timestamp is in the future, we'll crop it.
    const auto now = std::time(nullptr);
    if (ret > now)
        return now;

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
    {
        if (date.year < 100)
            date.year += 2000;
        return {true, date};
    }

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
    {
        if (date.year < 100)
            date.year += 2000;
        return {true, date};
    }


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

    if (ret.full)
    {
        if (date.year < 100)
            date.year += 2000;
        return {true, date};
    }
    return {false, date};
}

std::pair<bool, part> parse_part(const char* str, size_t len)
{
    nntp::part part {0};

    str = find_part_count(str, len, len);
    if (!str)
        return {false, part};

    using namespace boost::spirit::classic;

    // first notation (xx/yy)
    {
        const auto& ret = parse(str, str+len,
            (
               ch_p('(') >> uint_p[assign(part.numerator)] >>
                   ch_p('/') >> uint_p[assign(part.denominator)] >>
                   ch_p(')')
                   ));
        if (ret.full)
            return {ret.full, part};
    }

    // second notation  [xx/yy]
    const auto& ret = parse(str, str+len,
        (
         ch_p('[') >> uint_p[assign(part.numerator)] >>
             ch_p('/') >> uint_p[assign(part.denominator)] >>
             ch_p(']')
             ));
    return {ret.full, part};
}

void set_hash_function(hash_combine_function h)
{
    hash_combine = h;
}

std::uint32_t hashvalue(const char* subjectline, size_t len)
{
    std::size_t seed = 0;
    std::size_t skip = 0;
    const auto* p = find_part_count(subjectline, len, skip);
    if (!p)
    {
        for (std::size_t i=0; i<len; ++i)
            hash_combine(seed, subjectline[i]);
    }
    else
    {
        assert(p >= subjectline);
        const std::size_t num = p - subjectline;
        std::size_t i;
        for (i=0; i<num; ++i)
            hash_combine(seed, subjectline[i]);
        i += skip;
        for (; i<len; ++i)
            hash_combine(seed, subjectline[i]);
    }

    return std::uint32_t(seed);
}


std::string find_filename(const char* str, size_t len, bool include_extension)
{
    using namespace boost::spirit::classic;

    const static boost::regex regex(REVERSE_FILE_EXTENSION_REGEX, boost::regbase::icase | boost::regbase::perl);
    const static boost::regex mk1("File [0-9]* of [0-9]*");
    const static boost::regex mk2("(\\(|\\[)[0-9]*/[0-9]*(\\)|\\])");

    // see if it's an yEnc subjectline match.
    {
        std::string yenc_name;
        const auto ret = parse(str, str + len,
            (*(anychar_p - '"'))
                >> str_p("\"")
                >> (*(anychar_p - '"'))[assign(yenc_name)]
                >> str_p("\"")
                >> (*(anychar_p - '"'))
                >> str_p("yEnc")
                >> (*(anychar_p)));
        if (ret.hit)
        {
            if (include_extension)
                return yenc_name;
        }
        else if (ret.stop == str + len && !yenc_name.empty())
        {
            // something weird about the parse_info.
            // when the input is something like:
            // "8zm6tvYNq2.part14.rar" - 1.53GB <<< www.nfo-underground.xxx >>> yEnc (19/273)
            // then ret.hit == false, but however ret.stop == str + len
            // and the filaneme is stored into yenc_name.
            // I'm not really sure what's going on here and the documentaion
            // is so fucking super bad it's useless to even read.
            if (include_extension)
                return yenc_name;
        }

        using iterator = std::string::reverse_iterator;

        boost::match_results<iterator> res;
        if (regex_search(yenc_name.rbegin(), yenc_name.rend(), res, regex))
        {
            auto start = res[0].second;
            yenc_name.erase(start.base());
        }
    }

    reverse_c_str_iterator itbeg(str + len - 1);
    reverse_c_str_iterator itend(str - 1);

    boost::match_results<reverse_c_str_iterator> res;

    if (!regex_search(itbeg, itend, res, regex))
        return "";

    // first returns an iterator pointing to the
    // start of the sequence, which in this case is the end of the filename
    // extension.
    const char* ext = res[0].first.as_ptr();
    // second returns an iterator pointing one past the end of the sequence
    // which in this case is one before the the start of the filename extension
    const char* start = res[0].second.as_ptr();

    if (start < str)
        ++start;

    // seek the dot, its part of the regex so it must be found before start of str
    const char* dot = ext;
    while (*dot != '.')
    {
        if (--dot < start)
            return "";
    }

    if (dot == start)
        return "";

    if (dot[-1] == ' ')
        return "";

    const auto ext_len = ext - dot;

    char start_marker = 0;
    // look at the first character after the filename extension and see if it's a special
    // character such as ", ], )
    if (++ext < str + len)
    {
        if (*ext == '"')
            start_marker = '"';
        else if (*ext == ')')
            start_marker = '(';
        else if (*ext == ']')
            start_marker = '[';
        //else if (*ext == '#')
        //    start_marker = '#';
    }

    if (start_marker)
    {
        while (*start != start_marker && start > str)
            --start;

        // if we hit the marker exclude it from the name
        if (*start == start_marker)
            ++start;

        // trim whitespace
        while (std::isspace((unsigned char)*start) && start < dot)
            ++start;
    }
    else
    {
        // see if we can find some sort of upper limit how far to seek
        // forwards from the file extension
        // if the subject line contains something like
        // Foobar bla blah - File 07 of 10 - foobar.mp3" we use the "File xx of yy" as a marker
        {
            boost::match_results<const char*> res;
            if (regex_search(str, dot, res, mk1) ||
                regex_search(str, dot, res, mk2))
            {
                const auto e = res[0].second;
                const auto m = e - str;
                str  = e;
                len -= m;
            }
        }

        struct name_pred {

            name_pred(bool parity) : prev_(0), dash_(false), good_(true), parity_(parity)
            {}

            bool is_allowed(int c)
            {
                const auto ret = test(c);
                prev_ = c;
                return ret;
            }
            bool is_good()
            {
                return good_;
            }
            bool test(int c)
            {
                if (std::isalnum(c))
                {
                    if (prev_ == '-')
                        good_ = true;
                    return true;
                }

                if (c == '_' || c == '.' || c == ' ')
                {
                    return true;
                }
                else if (c == '-')
                {
                    if (prev_ == ' ' || prev_ == '-')
                    {
                        if (dash_ == true)
                            return false;

                        good_ = false;
                        dash_ = true;
                    }
                    return true;
                }
                else if (c == '+')
                {
                    if (parity_ && std::isdigit(prev_))
                        return true;
                }
                else if (c == ']' || c == ')' || c == '"')
                {
                    good_ = false;
                    enclosure_.push(c);
                    return true;
                }
                else if (c == '[')
                {
                    return test_top(']');
                }
                else if (c == '(')
                {
                    return test_top(')');
                }
                else if (c == '"')
                {
                    return test_top('"');
                }

                return false;
            }
        private:
            bool test_top(int expected)
            {
                if (enclosure_.empty())
                    return false;
                auto top = enclosure_.top();
                enclosure_.pop();
                good_ = top == expected;
                return good_;
            }

        private:
            int prev_;
            bool dash_;
            bool good_;
            bool parity_;
            std::stack<int> enclosure_;

        };
        bool is_parity = !strncmp(dot, ".PAR2", ext_len) || !strncmp(dot, ".par2", ext_len);

        name_pred pred(is_parity);

        const char* good = start;

        while (pred.is_allowed((unsigned char)*start) && start >= str)
        {
            if (pred.is_good())
                good = start;
            --start;
        }

        assert(good <= dot);

        while ((*good == ' ' || *good == '-') && (good < dot))
            ++good;

        start = good;
    }

    assert(start <= dot && dot < ext);

    if (start - dot == 0)
        return "";

    if (include_extension)
        return std::string(start, ext - start);

    return std::string(start, dot - start);
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

void thread_safe_initialize()
{
    // msvs doesn't implement magic static untill msvs2015
    // details here;
    // https://msdn.microsoft.com/en-us/library/hh567368.aspx

    std::size_t i;
    find_part_count("", 0, i);
    is_binary_post("", 0);
    find_filename("", 0, false);
}

} // nntp
