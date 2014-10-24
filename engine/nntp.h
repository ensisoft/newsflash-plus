// Copyright (c) 2013,2014 Sami Väisänen, Ensisoft 
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
//  THE SOFTWARE

// $Id: nntp.h,v 1.53 2010/07/04 18:33:53 svaisane Exp $

#pragma once

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <boost/algorithm/string/trim.hpp>
#include <newsflash/warnpop.h>
#include <sstream>
#include <iomanip>
#include <string>
#include <limits>
#include <ctime>
#include <cstddef>
#include <cstdint>
#include <exception>

namespace nntp
{
    // broken down Usenet article overview fields.
    struct overview {
        struct field {
            const char* start;
            size_t len;
        };
        field number;
        field subject;
        field author;
        field date;
        field messageid;
        field references;
        field bytecount;
        field linecount;
    };

    // the broken down date and time of a Usenet article
    struct date {
        int day;
        int year;
        int hours;
        int minutes;
        int seconds;
        int tzoffset;
        std::string month;
        std::string tz;
    };

    struct group {
        std::string name;
        std::string first;
        std::string last;
        char   posting;
    };

    // (01/34)
    struct part {
        std::size_t numerator;  // 1        
        std::size_t denominator; // 34
    };

    // inspect the subject line and try to figure out whether
    // it indicates that post contains binary data.
    bool is_binary_post(const char* str, size_t len);

    // convert a broken down date object into a single scalar integral value.
    // the returned values are all universally comparable since they're all
    // converted to GMT times. They are also usable with C time functions
    // as long as one keeps in mind that that they time they represent is in GMT
    std::time_t timevalue(const nntp::date& date);

    // parse overview, returns (true, overview) if successful,
    // otherwise (false, overview) and the contents of response are undefined.
    std::pair<bool, overview> parse_overview(const char* str, size_t len);

    // parse group information. returns (true, group) if succesful,
    // otherwise (false, overview) and the contents of group are undefined.
    std::pair<bool, group> parse_group(const char* str, size_t len);

    // parse nntp date. returns (true, date) if succesful,
    // otherwise (false, date) and the contents of date are undefined
    std::pair<bool, date> parse_date(const char* str, size_t len);

    // parse subject line part information.
    std::pair<bool, part> parse_part(const char* str, size_t len);

    // return a 32bit hash value representing this subject line
    // this function ensures that "foobar.mp3 (1/10)" and "foobar.mp3 (2/10)" hash to the same value.
    std::uint32_t hashvalue(const char* subjectline, size_t len);

    // find a filename in the given subjectline. if no filename was found
    // then returns (nullptr, 0), otherwise a pointer to the start of the filename
    // and the length of the name. 
    // example: "South.park "Southpark.S1E3.par2" yEnc" returns ("southpark.S1E3.par2", 19)
    std::string find_filename(const char* str, size_t len);

    inline
    std::string find_filename(const std::string& s)
    { return find_filename(s.c_str(), s.size()); }


    // scan the given buffer for a complete nntp response.
    // returns the length of the response if found including the terminating sequence.
    // if no response is found returns 0.
    std::size_t find_response(const void* buff, std::size_t size);

    // scan the given buffer for a complete message body.
    // returns the length of the body if found including the terminating sequence.
    // if no body is found returns 0.
    std::size_t find_body(const void* buff, std::size_t size);

    struct trailing_comment {
        std::string str;
    };

    namespace detail {
        template<typename T>
        void extract_value(std::stringstream& ss, T& value)
        {
            ss >> std::skipws >> value;
        }

        inline
        void extract_value(std::stringstream& ss, std::string& str)
        {
            ss >> std::skipws >> str;
        }

        inline
        void extract_value(std::stringstream& ss, trailing_comment& comment)
        {
            std::getline(ss, comment.str);
            boost::algorithm::trim(comment.str);
        }

        template<typename Value>
        void scan_next_value(std::stringstream& ss, Value& next)
        {
            extract_value(ss, next);
        }

        template<typename Value, typename... Rest>
        void scan_next_value(std::stringstream& ss, Value& next, Rest&... gang)
        {
            extract_value(ss, next);
            scan_next_value(ss, gang...);
        }

    } // detail

    // gets thrown when things go wrong
    class exception : public std::exception
    {
    public:
        exception(std::string what, int response_code) NOTHROW : what_(std::move(what)), code_(response_code)
        {}
        const char* what() const NOTHROW
        { return what_.c_str(); }

        int code() const NOTHROW
        { return code_; }

    private:
        std::string what_;
        int code_;
    };


    // scan response string for a sequence of values given their specific type. 
    // returns true if scan was succesful, otherwise false.
    template<typename... Values>
    int scan_response(std::vector<int> codes, const char* response, std::size_t len, Values&... values)
    {
        std::string str(response, len);
        std::stringstream ss(str);
        int code = 0;
        detail::extract_value(ss, code);
        if (std::find(std::begin(codes), std::end(codes), code) == std::end(codes))
            throw nntp::exception("incorrect response code: " + str, code);
        detail::scan_next_value(ss, values...);
        if (ss.fail())
            throw nntp::exception("scan response failed: " + str, 0);

        return code;
    }

    inline
    int scan_response(std::vector<int> codes, const char* response, std::size_t len)
    {
        std::string str(response, len);
        std::stringstream ss(str);
        int code = 0;
        detail::extract_value(ss, code);
        if (ss.fail())
            throw nntp::exception("scan response failed: " + str, 0);
        
        if (std::find(std::begin(codes), std::end(codes), code) == std::end(codes))
            throw nntp::exception("incorrect response code: " + str, code);

        return code;
    }


} // nntp
