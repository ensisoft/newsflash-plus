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

// $Id: file.h,v 1.11 2010/02/25 13:12:40 svaisane Exp $ 

#pragma once

#include <system_error>
#include <cstdint>
#include <string>
#include <memory>
#include <utility>

namespace newsflash
{
    // bigfile handles big files up to 2^63-1 bytes in size
    class bigfile
    {
    public:
        typedef int64_t big_t;

        bigfile();
       ~bigfile();

        // Try to open an existing file. Returns 0 on success
        // or a platform error code on error.
        std::error_code open(const std::string& file);

        // Open a file for appending. If file doesn't exist
        // it's created, otherwise it's opened for appending.
        // A file opened for appending will *always* append
        // regardless of current seek position.
        // returns 0 on success or a platform error code on error.
        std::error_code append(const std::string& file);

        // Create a new file. If the file already exists the contents
        // are truncated to 0. 
        // Returns 0 on success or a platform error code on error.
        std::error_code create(const std::string& file);

        // check if already open, returns true if open otherwise false.
        bool is_open() const;

        // close an already opened file.
        void close();

        // get current file pointer poisition in the file.
        big_t position() const;

        // Seek to a file location specified by offset.
        // Seeking always occurs from the start of the file.
        void seek(big_t offset);

        // Try to write the given number of bytes to the file.
        void write(const void* data, size_t bytes);

        // Try to read the given number of bytes from the file
        // returns the actual number of bytes read. 
        size_t read(void* buff, size_t bytes);

        // flush buffered writes to the file on device.
        void flush();

        // get file size. 
        static std::pair<std::error_code, big_t> size(const std::string& file);

        // erase the file. returns 0 on success or 
        // platform specific native error code on error.
        static std::error_code erase(const std::string& file);

        // Resize the file (either truncate or expand) to the given size. 
        static std::error_code resize(const std::string& file, big_t size);

        // check if file exists or not.
        static bool exists(const std::string& file);
    private:
        struct impl;

        std::unique_ptr<impl> pimpl_;
    };

} // newsflash



