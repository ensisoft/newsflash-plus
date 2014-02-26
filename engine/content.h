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

#include <memory>
#include <vector>
#include <map>
#include "decoder.h"
#include "bigfile.h"
#include "stopwatch.h"

namespace newsflash
{
    class buffer;

    // content decoded from a sequence of buffers and 
    // stored to a file on the filesystem
    class content
    {
    public:
        content(std::string folder,
                std::string initial_file_name,
                std::unique_ptr<decoder> dec, 
                bool overwrite = true);

       ~content();

        // decode the contents of the buffer
        void decode(std::shared_ptr<const buffer> buff);

        // cancel and delete any objects stored on the disk.
        void cancel();

        // finalize the content. assemble any non-decoded pieces
        // together and write any remainig data to the disk.
        void finish();

        bool good() const;

    private:
        void on_error(const std::string& err);                
        void on_info(const decoder::info& info);
        void on_write(const void* data, std::size_t size, std::size_t offset);

    private:
        typedef std::map<std::shared_ptr<const buffer>, std::size_t> buffmap;

    private:
        std::size_t next_buffer_id_;
        std::string path_;
        std::string name_;        
        std::unique_ptr<decoder> decoder_;    
        std::vector<std::string> errors_;
        bigfile file_;
        buffmap buffers_;
        stopwatch watch_;        

        bool overwrite_;

        #ifdef NEWSFLASH_DEBUG
        bool completed_;
        #endif
    };

} // newsflash
