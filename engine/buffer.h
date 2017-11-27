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
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstring>

#include "utility.h"

namespace newsflash
{
    // NNTP data buffer. the buffer contents are split into 2 segments
    // the payload (body) and the response line that preceeds the data
    class Buffer
    {
    public:
        // The type of the buffer content.
        enum class Type {
            // empty buffer, no type specified.
            None,

            // content is XOVER (or XZVER) data overview data.
            Overview,

            // content is BODY article data.
            Article,

            // content is LIST newsgroup listing data.
            GroupList,

            // contne is GROUP newsgroup data
            GroupInfo
        };

        // status of the buffer
        enum class Status {
            // empty buffer, no status.
            None,

            // data was succesfully received
            Success,

            // data was not available.
            Unavailable,

            // data was not available because it was dmca taken down
            Dmca,

            // there was an error reading the data
            // for example XZVER zlib deflate failed.
            Error
        };

        using u8 = char;

        Buffer(std::size_t initial_capacity)
        {
            buffer_.resize(initial_capacity);
        }

        Buffer()
        {}

        Buffer(Buffer&& other) : buffer_(std::move(other.buffer_))
        {
            size_   = other.size_;
            type_   = other.type_;
            status_ = other.status_;
            content_start_  = other.content_start_;
            content_length_ = other.content_length_;
        }
        Buffer(const Buffer& other) : buffer_(other.buffer_)
        {
            size_   = other.size_;
            type_   = other.type_;
            status_ = other.status_;
            content_start_  = other.content_start_;
            content_length_ = other.content_length_;
        }

        // return content pointer to the start of the body/payload data
        const u8* Content() const
        { return &buffer_[content_start_]; }

        // return head pointer to the start of the whole buffer
        const u8* Head() const
        { return &buffer_[0]; }

        // return back pointer for writing data
        u8* Back()
        { return &buffer_[size_]; }

        // after writing to the back() pointer, commit the number
        // of bytes written
        void Append(std::size_t size)
        {
            assert(size + size_  <= buffer_.size());
            size_ += size;
        }

        void Append(const char* str)
        {
            assert(std::strlen(str) + size_ + 1 <= buffer_.size());
            std::strcpy(Back(), str);
            size_ += std::strlen(str);
        }

        void Append(const std::string& s)
        {
            Append(s.c_str());
        }

        void Allocate(std::size_t capacity)
        {
            if (capacity < buffer_.size())
                return;

            buffer_.resize(capacity);
        }

        void Grow(std::size_t num_bytes)
        {
            Allocate(GetSize() + num_bytes);
        }

        void Clear()
        {
            size_ = 0;
            content_start_  = 0;
            content_length_ = 0;
            type_   = Type::None;
            status_ = Status::None;
        }

        void Pop(std::size_t point)
        {
            assert(point <= size_);
            //const auto bytes_to_copy = point;
            const auto bytes_to_move = size_ - point;

            std::memmove(&buffer_[0], &buffer_[point], bytes_to_move);
            size_ = bytes_to_move;
            return;
        }

        // split the buffer into two buffers at the specified splitpoint.
        // the size, capacity and the contents of the split buffer are those
        // of this buffer from 0 to splitpoint.
        // after the split the contents of this buffer are shifted to 0
        // and current size is decreased by splitpoint bytes.
        Buffer Split(std::size_t splitpoint)
        {
            assert(splitpoint <= size_);

            const auto bytes_to_copy = splitpoint;
            const auto bytes_to_move = size_ - splitpoint;

            Buffer ret(bytes_to_copy);

            std::memcpy(&ret.buffer_[0],&buffer_[0], bytes_to_copy);
            std::memmove(&buffer_[0], &buffer_[splitpoint], bytes_to_move);

            ret.size_ = bytes_to_copy;
            size_ = bytes_to_move;
            return ret;
        }

        // set the buffer status
        void SetStatus(Status status)
        { status_ = status; }

        // set content type
        void SetContentType(Type type)
        { type_ = type;}

        // the buffer can contain both the simple nntp response header
        // and the actual data content. instead of removing the header
        // we just specificy a start offset for the content to begin in the buffer
        void SetContentStart(std::size_t start)
        {
            assert(start + content_length_ <= size_);
            content_start_ = start;
        }

        // set the actual length of the content data.
        void SetContentLength(std::size_t length)
        {
            assert(content_start_ + length <= size_);
            content_length_ = length;
        }

        // get the type of the buffer content.
        Type GetContentType() const
        { return type_; }

        // get the status of the buffer content.
        Status GetContentStatus() const
        { return status_; }

        // return the size of the whole buffer
        std::size_t GetSize() const
        { return size_; }

        // get the total capacity of the buffer
        std::size_t GetCapacity() const
        { return buffer_.size(); }

        // get the length of the content
        std::size_t GetContentLength() const
        { return content_length_; }

        // get the offset into the buffer where the
        // actual content starts.
        std::size_t GetContentStart() const
        { return content_start_; }

        // return how many bytes are available for appending through back() pointer
        std::size_t GetAvailableBytes() const
        { return buffer_.size() - size_; }

        // returns true if buffer is full, i.e.
        // GetAvailableBytes is zero.
        bool IsFull() const
        { return GetAvailableBytes() == 0; }


        Buffer& operator=(Buffer&& other)
        {
            Buffer tmp(std::move(*this));

            buffer_         = std::move(other.buffer_);
            size_           = other.size_;
            content_start_  = other.content_start_;
            content_length_ = other.content_length_;
            type_   = other.type_;
            status_ = other.status_;
            return *this;
        }

        Buffer& operator=(const Buffer& other)
        {
            if (this == &other)
                return *this;

            buffer_ = other.buffer_;
            size_   = other.size_;
            content_start_  = other.content_start_;
            content_length_ = other.content_length_;
            type_   = other.type_;
            status_ = other.status_;
            return *this;
        }

    private:
        std::vector<u8> buffer_;
        std::size_t size_ = 0;
        std::size_t content_start_  = 0;
        std::size_t content_length_ = 0;
    private:
        Type type_ = Type::None;
        Status status_ = Status::None;
    };
} // newsflash
