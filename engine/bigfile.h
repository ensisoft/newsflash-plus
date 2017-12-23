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

#pragma once

#include "newsflash/config.h"

#include <system_error>
#include <cstdint>
#include <string>
#include <memory>

#include "utility.h"

namespace newsflash
{
    // bigfile handles big files up to 2^63-1 bytes in size
    class bigfile : noncopyable
    {
    public:
        enum open_flags {
            o_no_flags = 0,
            o_create   = 1 << 0, // create file if it doesnt exist
            o_truncate = 1 << 1, // truncate file contents to 0 length on open
            o_append   = 1 << 2  // always append to the file.
        };

        typedef std::int64_t big_t;

        // create an unopened bigfile object
        bigfile();

        // construct a a bigfile object and try to open the given file
        bigfile(const std::string& file, unsigned flags = 0);
        bigfile(bigfile&& other);

       ~bigfile();

        // Open a file.
        void open(const std::string& file, unsigned flags = 0,
            std::error_code* error = nullptr);

        // check if already open, returns true if open otherwise false.
        bool is_open() const;

        // close an already opened file.
        void close();

        // get current file pointer poisition in the file.
        big_t position() const;

        // get current file size
        big_t size() const;

        //void resize(big_t size);

        // Seek to a file location specified by offset.
        // Seeking always occurs from the start of the file.
        void seek(big_t offset);

        //void resize(big_t size);

        // Try to write the given number of bytes to the file.
        void write(const void* data, std::size_t bytes);

        // Try to read the given number of bytes from the file
        // returns the actual number of bytes read.
        std::size_t read(void* buff, std::size_t bytes);

        // flush buffered writes to the file on device.
        void flush();

        bigfile& operator=(bigfile&& other);

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



