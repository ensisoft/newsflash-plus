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

#include <newsflash/config.h>
#include <memory>
#include <vector>
#include <string>

namespace newsflash
{
    class filebuf
    {
        class fileio;

    public:
        using byte = unsigned char;

        class buffer
        {
        public:
            using iterator = std::vector<byte>::iterator;
            using const_iterator = std::vector<byte>::const_iterator;

            buffer()
            {}

            iterator begin()
            {
                return data_.begin();
            }

            const_iterator begin() const
            {
                return data_.begin();
            }

            iterator end()
            {
                return data_.end();
            }
            const_iterator end() const
            {
                return data_.end();
            }

            void* address()
            {
                return data_.data();
            }
            const void* address() const
            {
                return data_.data();
            }

            std::size_t length() const
            {
                return data_.size();
            }
            void flush();

        private:
            buffer(std::shared_ptr<fileio> io, std::vector<byte> data,
                std::size_t offset, bool write) : fileio_(io), data_(std::move(data)), offset_(offset), write_(write)
            {}
            friend class filebuf;
        private:
            std::shared_ptr<fileio> fileio_;
            std::vector<byte> data_;
            std::size_t offset_;
        private:
            bool write_;
        };

        // open the filebuf based on the given file.
        // if the file doesn't exist it's created.
        // on error an exception is thrown.
        void open(const std::string& filename);

        // close the current filebuf.
        void close();

        enum buffer_flags {
            buf_read  = 1 << 0,
            buf_write = 1 << 1
        };

        // load a buffer of the given size (in bytes) from the
        // filebuf from the  given offset.
        buffer load(std::size_t offset, std::size_t size, unsigned flags);

        // get the size of the file backing the filebuf.
        std::size_t size() const;

        // get the current name of the file.
        std::string filename() const;

        // returns true if currently open.
        bool is_open() const;
    private:
        std::shared_ptr<fileio> fileio_;
        std::string filename_;
    };
} // newsflash