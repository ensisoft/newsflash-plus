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

#include <exception>
#include <string>
#include <functional>

namespace newsflash
{
    // NNTP data decoder. extracts/decodes ascii armoured binaries 
    // out of NNTP data buffers. 
    class decoder
    {
    public:
        // unexpected error. decoding cannot continue.
        class exception : public std::exception
        {
        public:
            exception(std::string what) NOTHROW 
                : what_(std::move(what))
            {} 

            const char* what() const NOTHROW
            {
                return what_.c_str();
            }
        private:
            const std::string what_;
        };

        enum class error {
            crc, size, partial_content
        };

        struct info {
            std::string name; // decoded binary name
            std::size_t size; // expected decoded binary size
        };

        // info callback when meta information is available.
        std::function<void (const decoder::info& info)> on_info;

        // write a chunk of decoded data
        std::function<void (const void* data, std::size_t size, std::size_t offset, bool has_offset) > on_write;

        // a decoding error was encountered (for example incorrect CRC32)
        // the binary may be broken.
        std::function<void (decoder::error error, const std::string& str)> on_error;

        virtual ~decoder() = default;

        // decode the data buffer. 
        // returns the number of bytes consumed.
        virtual std::size_t decode(const void* data, std::size_t len) = 0;

        // finish the decoding session. no more buffers coming. 
        virtual void finish() = 0;
    protected:
    private:
    };

} // newsflash
