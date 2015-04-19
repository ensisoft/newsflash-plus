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

#pragma once

#include <newsflash/config.h>

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

namespace newsflash
{
    namespace ui {

    // a download job details for downloading content
    struct download
    {
        // list of message id's or message numbers.
        // note that message numbers are specific to a server
        // while message-ids are portable across servers.
        std::vector<std::string> articles;

        // the list of groups into which look for the messge ids.
        std::vector<std::string> groups;

        // the esimated size of the file to be downloaded in bytes. 
        // 0 if not known.
        std::uint64_t size;                    

        // the local filesystem path where the downloadded/decoded content
        // is to the be placed.
        std::string path;

        // the human readable description that will appear in task::description.
        // usually the expected name of the file.
        std::string name;
    };

    // message-id's are unique across all Usenet servers and have are variable
    // length strings enclosed in angle brackets '<' and '>'. 
    // if the articles in the download are message-ids we the download is 
    // "fillable", i.e. another server can queried for the messages in case
    // they are missing from the primary server. 
    inline bool is_fillable(const download& d) 
    {
        for (const auto& a : d.articles)
        {
            if (a.front() != '<' || a.back() != '>')
                return false;
        }
        return true;
    }

} // ui
} // engine
