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

#include <vector>
#include <string>
#include <cstddef>

namespace engine
{
    // a file available in usenet network. each file
    // is identified by a list of message-ids or message numbers.
    struct file
    {
        // list of message id's or message numbers.
        // note that message numbers are specific to a server
        // while message-ids are portable across servers.
        std::vector<std::string> ids;

        // the list of groups into which look for the messge ids.
        std::vector<std::string> groups;

        // the local filesystem path where the content
        // is to the be placed.
        std::string path;

        std::string name;

        // the human readable description. 
        // this will appear in task::description.
        std::string description;

        // the esimated size of the content to be downloaded in bytes. 
        std::uint64_t size;

        // the account to be used for downloading the content.
        std::size_t account;

        // contents are suspected to be damaged
        bool damaged;
    };

} // engine
