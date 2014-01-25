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
#include <cstdint>

namespace newsflash
{
    // interface for listening to engine events.
    // the callback functions should be nothrow exception safe. 
    class listener 
    {
    public:
        typedef uint32_t errcode_t; // native system spefic error code

        enum class object {
            socket, file
        };

        struct error {
            object obj;
            errcode_t err;
            std::string msg;
            std::string resource;
        };

        virtual ~listener() = default;

        // this is invoked when an error has happened.
        // for example socket failed to connect, file could not
        // be created or opened etc.
        virtual void on_error(const error& e) = 0;

        // engine has finished performing an orderly shutdown.
        virtual void on_shutdown() = 0;

        //virtual void on_listing_complete()
        struct file {
            enum class status {
                success, dmca, unavailable, broken
            };
            status stat;
            std::string name;
            std::size_t account;
            std::uint64_t size;
        };

        struct filegroup {
            std::size_t account;
            std::size_t id;         
            std::string location;               
            std::vector<file> files;
        };

        // an independent file outside any filegroup was completed.
        virtual void on_complete(const file& f) = 0;

        // a file within a group was completed.
        //virtual void on_file_complete(const file& f, const filegroup& files) = 0;

        // a group of files was completed.
        virtual void on_complete(const filegroup& files) = 0;
    protected:
    private:
    };

} // newsflash

