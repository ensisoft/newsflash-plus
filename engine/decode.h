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

#include <stdexcept>
#include <string>
#include <vector>

#include "action.h"
#include "buffer.h"
#include "encoding.h"
#include "bitflag.h"

namespace newsflash
{
    // implement data decoding action. I.e. read input NNTP data and extract and decode it.
    // any ascii armored binary
    class DecodeJob : public action
    {
    public:
        enum class Encoding {
            yEnc,
            UUEncode,
            Unknown
        };

        // gets throw when decoding has encountered
        // an unrecoverable error and to continue is impossible
        class Exception : public std::exception
        {
        public:
            Exception(const std::string& what) : what_(what)
            {}

            const char* what() const NOTHROW // noexcept
            { return what_.c_str(); }

        private:
            std::string what_;
        };

        // a soft error. the binary might be unsable
        enum class Error {
            CrcMismatch,
            SizeMismatch
        };

        DecodeJob(const Buffer& data);
        DecodeJob(Buffer&& data);

        virtual std::string describe() const override;

        // get text content buffer (if any)

        // doesn't work with msvc!
        // std::vector<char>&& get_text_data() &&

        std::vector<char>&& GetTextDataMove()
        { return std::move(text_); }

        // get the binary content (if any)
        //std::vector<char>&& get_binary_data() &&

        std::vector<char>&& GetBinaryDataMove()
        { return std::move(binary_); }

        const std::vector<char>& GetTextData() const //&
        { return text_; }

        const std::vector<char>& GetBinaryData() const //&
        { return binary_; }

        // if binary is part of a multipart binary then
        // it might have a specific offset in the final output binary.
        // the caller should use this value only with encodings that
        // support such a scheme. (i.e. yenc multi)
        std::size_t GetBinaryOffset() const
        { return binary_offset_; }

        // get the binary size. note that in case of a multipart binary
        // that supports this, the value reported is the *whole* binary
        // size, not the size of the current part.
        std::size_t GetBinarySize() const
        { return binary_size_; }

        // get the binary file name in UTF-8
        std::string GetBinaryName() const
        { return binary_name_; }

        // get error flags set after the decoding.
        bitflag<Error> GetErrors() const
        { return errors_; }

        // get the identified encoding. see comments in get_binary_offset
        Encoding GetEncoding() const
        { return encoding_; }

        bool IsMultipart() const
        { return flags_.test(Flags::Multipart); }

        bool IsFirstPart() const
        { return flags_.test(Flags::FirstPart); }

        bool IsLastPart() const
        { return flags_.test(Flags::LastPart); }

        bool HasOffset() const
        { return flags_.test(Flags::HasOffset); }

    private:
        virtual void xperform() override;
        std::size_t decode_yenc_single(const char* data, std::size_t len);
        std::size_t decode_yenc_multi(const char* data, std::size_t len);
        std::size_t decode_uuencode_single(const char* data, std::size_t len);
        std::size_t decode_uuencode_multi(const char* data, std::size_t len);

    private:
        Buffer data_;
        std::vector<char> text_;
        std::vector<char> binary_;
        std::size_t binary_offset_ = 0;
        std::size_t binary_size_   = 0;
        std::string binary_name_;
    private:
        Encoding encoding_ = Encoding::Unknown;
    private:
        enum class Flags {
            Multipart,
            FirstPart,
            LastPart,
            HasOffset
        };
        bitflag<Flags> flags_;
        bitflag<Error> errors_;
    private:
    };

} // newsflash
