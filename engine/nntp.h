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
//  THE SOFTWARE.

// $Id: nntp.h,v 1.53 2010/07/04 18:33:53 svaisane Exp $

#pragma once

#include <newsflash/config.h>

#include <vector>
#include <string>
#include <cassert>
#include <iterator>
#include <limits>
#include <ctime>
#include <cstddef>
#include <cstdint>

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

    struct part {
        // (01/34)
        size_t numerator;  // 1        
        size_t denominator; // 34
    };

    // inspect the subject line and try to figure out whether
    // it indicates that post contains binary data.
    bool is_binary_post(const char* str, size_t len);

    // convert a broken down date object into a single scalar integral value.
    // the returned values are all universally comparable since they're all
    // converted to GMT times. They are also usable with C time functions
    // as long as one keeps in mind that that they time they represent is in GMT
    std::time_t timevalue(const date& d);

    // parse overview, returns (true, overview) if successful,
    // otherwise (false, overview) and the contents of response are undefined.
    std::pair<bool, overview> parse_overview(const char* str, size_t len);

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
    std::pair<const char*, size_t> find_filename(const char* str, size_t len);

} // nntp
