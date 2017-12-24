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

#include <cstdint>
#include <cstring>
#include <ctime>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <type_traits>

#include "stringlib/string_view.h"
#include "bitflag.h"
#include "filemap.h"
#include "filetype.h"
#include "assert.h"
#include "nntp.h"
#include "utf8.h"
#include "iso_8859_15.h"

namespace newsflash
{
    template<typename Storage>
    class article
    {
    public:
        article()
        {
            clear();
        }

        bool parse(const char* overview, std::size_t len)
        {
            const auto& pair = nntp::parse_overview(overview, len);
            if (!pair.first)
                return false;

            const auto& data = pair.second;
            if (data.subject.len == 0 || data.subject.len > 512)
                return false;
            if (data.author.len == 0)
                return false;

            const auto& part = nntp::parse_part(data.subject.start, data.subject.len);
            if (part.first)
            {
                if (part.second.numerator > part.second.denominator)
                    return false;
                m_parts_avail = 1;
                m_partno      = part.second.numerator;
                m_parts_total = part.second.denominator;
                m_bits.set(fileflag::broken, (m_parts_avail != m_parts_total));
            }

            const auto date = nntp::parse_date(data.date.start, data.date.len);
            if (!date.first)
                return false;

            m_pubdate = nntp::timevalue(date.second);

            const auto& filename = nntp::find_filename(data.subject.start, data.subject.len);
            if (!filename.empty())
            {
                m_type = find_filetype(filename);
                m_bits.set(fileflag::binary);
            }

            // we support utf8 now.
            // using runtime article specific flag here insted of versioning the catalog
            // so that we can support backwards compatibility with old data.
            // we can break this on a major release and refactor to more efficient
            // implementation runtime wise.
            m_bits.set(fileflag::enable_utf8);

            // if the subject string is well formed UTF-8 we're going to store it as is
            // otherwise we expect that it's in extended latin. (actually according to spec
            // latin-1 but in reality this isn't the case)
            if (utf8::is_well_formed(data.subject.start, data.subject.start + data.subject.len))
                m_subject = std::string{data.subject.start, data.subject.len};
            else  m_subject = ISO_8859_15_to_utf8(data.subject.start, data.subject.len);

            if (utf8::is_well_formed(data.author.start, data.author.start + data.author.len))
                m_author  = std::string{data.author.start, data.author.len};
            else m_author = ISO_8859_15_to_utf8(data.author.start, data.author.len);

            m_number  = nntp::to_int<std::uint64_t>(data.number.start, data.number.len);
            m_bytes   = nntp::to_int<std::uint32_t>(data.bytecount.start, data.bytecount.len);

            // with the addition of utf-8 in the subject line we must cater for cases
            // where existing data is updated. so instead of calculating the hash from the utf-8
            // subject line we do it as before, i.e. from the subject line.
            //m_hash    = nntp::hashvalue(m_subject.c_str(), m_subject.size());
            m_hash = nntp::hashvalue(data.subject.start, data.subject.len);

            if (m_author.size() > 64)
                m_author.resize(64);

            return true;
        }

        // load the article data from the specified offset in the storage object.
        void load(std::size_t offset, Storage& storage)
        {
            const auto size  = storage.size();
            const auto avail = size - offset;
            const auto min   = std::min<std::size_t>(avail, 1024);
            assert(offset < size);

            auto buffer = storage.load(offset, min, Storage::buf_read);

            auto in = buffer.begin();
            read(in, m_index);
            read(in, m_bytes);
            read(in, m_idbkey);
            read(in, m_parts_avail);
            read(in, m_parts_total);
            read(in, m_number);
            read(in, m_pubdate);
            read(in, m_bits);
            read(in, m_type);
            read(in, m_subject);
            read(in, m_author);
            auto m = MAGIC;
            read(in, m);
            if (m != MAGIC)
                throw std::runtime_error("article read error. no magic found");
        }

        // save the article at a specific location (offset) in the storage object.
        void save(std::size_t offset, Storage& storage) const
        {
            auto buffer = storage.load(offset, size_on_disk(), Storage::buf_write);
            auto out = buffer.begin();
            write(out, m_index);
            write(out, m_bytes);
            write(out, m_idbkey);
            write(out, m_parts_avail);
            write(out, m_parts_total);
            write(out, m_number);
            write(out, m_pubdate);
            write(out, m_bits);
            write(out, m_type);
            write(out, m_subject);
            write(out, m_author);
            write(out, MAGIC);
            buffer.write();
        }

        void combine(const article& other)
        {
            m_bytes += other.m_bytes;
            if (has_parts())
            {
                m_parts_avail++;
                m_bits.set(fileflag::broken, (m_parts_total != m_parts_avail));
            }
        }

        std::size_t size_on_disk() const
        {
            return sizeof(m_bits) +
                sizeof(m_type) +
                sizeof(m_index) +
                sizeof(m_bytes) +
                sizeof(m_idbkey) +
                sizeof(m_parts_avail) +
                sizeof(m_parts_total) +
                sizeof(m_number) +
                sizeof(m_pubdate) +
                2 + m_subject.size() +
                2 + m_author.size() +
                sizeof(MAGIC);
        }

        void clear()
        {
            m_bits.clear();
            m_type        = filetype::none;
            m_index       = 0;
            m_bytes       = 0;
            m_idbkey      = 0;
            m_parts_avail = 0;
            m_parts_total = 0;
            m_partno      = 0;
            m_number      = 0;
            m_pubdate     = 0;
            m_subject.clear();
            m_author.clear();
            m_hash   = 0;
            m_partno = 0;
        }

        bitflag<fileflag, std::uint8_t> bits() const
        { return m_bits; }

        std::uint32_t index() const
        { return m_index; }

        std::uint32_t bytes() const
        { return m_bytes; }

        std::uint64_t idbkey() const
        { return m_idbkey; }

        std::uint16_t num_parts_total() const
        { return m_parts_total; }

        std::uint16_t num_parts_avail() const
        { return m_parts_avail; }

        std::uint16_t partno() const
        { return m_partno; }

        std::uint64_t number() const
        { return m_number; }

        std::time_t pubdate() const
        { return m_pubdate; }

        const std::string& subject() const
        { return m_subject; }

        const std::string& author() const
        { return m_author; }

        std::string subject_as_string() const
        { return m_subject; }

        filetype type() const
        { return m_type; }

        const std::uint32_t hash() const
        {
            if (!m_hash)
                m_hash = nntp::hashvalue(m_subject.c_str(), m_subject.size());
            return m_hash;
        }

        bool test(fileflag flag) const
        {
            return m_bits.test(flag);
        }

        void set_author(const char* str)
        {
            m_author = str;
        }
        void set_author(const char* str, std::size_t len)
        {
            m_author = std::string{str, len};
        }

        void set_subject(const char* str)
        {
            m_subject = str;
        }
        void set_subject(const char* str, std::size_t len)
        {
            m_subject = std::string{str, len};
        }

        void set_bytes(std::uint32_t bytes)
        {
            m_bytes = bytes;
        }
        void set_number(std::uint64_t number)
        {
            m_number = number;
        }
        void set_bits(fileflag flag, bool on_off)
        {
            m_bits.set(flag, on_off);
        }
        void set_idbkey(std::uint64_t key)
        {
            m_idbkey = key;
        }
        void set_pubdate(std::time_t pub)
        {
            m_pubdate = pub;
        }

        void set_index(std::uint32_t index)
        {
            m_index = index;
        }

        bool is_match(const article& other) const
        {
            if (m_bits.test(fileflag::enable_utf8) && other.m_bits.test(fileflag::enable_utf8))
                return nntp::strcmp(m_subject, other.m_subject);

            if (!m_bits.test(fileflag::enable_utf8) && !other.m_bits.test(fileflag::enable_utf8))
                return nntp::strcmp(m_subject, other.m_subject);

            // mixed case. old and new data.
            if (!m_bits.test(fileflag::enable_utf8))
                return nntp::strcmp(ISO_8859_15_to_utf8(m_subject), other.m_subject);

            return nntp::strcmp(m_subject, ISO_8859_15_to_utf8(other.m_subject));

        }

        bool is_broken() const
        {
            return m_bits.test(fileflag::broken);
        }

        bool is_deleted() const
        {
            return m_bits.test(fileflag::deleted);
        }

        bool has_parts() const
        {
            return bool(m_parts_total != 0);
        }
        bool is_utf8_enabled() const
        {
            return m_bits.test(fileflag::enable_utf8);
        }

    private:
        typedef typename Storage::buffer buffer;
        typedef typename Storage::buffer::iterator iterator;
        typedef typename Storage::buffer::const_iterator const_iterator;

        template<typename Value>
        void read(iterator& it, Value& val) const
        {
            // this should be std::is_trivially_copyable (not available in gcc 4.9.2)
            static_assert(std::is_standard_layout<Value>::value, "");

            auto* p = (char*)&val;
            for (std::size_t i=0; i<sizeof(val); ++i)
                p[i] = *it++;
        }

        void read(iterator& it, str::string_view& s) const
        {
            std::uint16_t len;
            read(it, len);
            const auto ptr = (const char*)&(*it);
            it += len;
            s = str::string_view{ptr, len};
        }
        void read(iterator& it, std::string& s) const
        {
            std::uint16_t len;
            read(it, len);
            const auto ptr = (const char*)&(*it);
            it += len;
            s = std::string(ptr, len);
        }

        template<typename Value>
        void write(iterator& it, const Value& val) const
        {
            // this should be std::is_trivially_copyable (not available in gcc 4.9.2)
            static_assert(std::is_standard_layout<Value>::value, "");

            const auto* p = (const char*)&val;
            for (std::size_t i=0; i<sizeof(val); ++i)
                *it++ = p[i];
        }

        void write(iterator& it, const str::string_view& str) const
        {
            const std::uint16_t len = str.size();
            write(it, len);
            std::copy(str.begin(), str.end(), it);
            it += len;
        }
        void write(iterator& it, const std::string& str) const
        {
            const std::uint16_t len = str.size();
            write(it, len);
            std::copy(str.begin(), str.end(), it);
            it += len;
        }

    private:
        static const std::uint32_t MAGIC;

    private:
        bitflag<fileflag, std::uint8_t> m_bits;
        filetype m_type;

        std::uint32_t m_index;
        std::uint32_t m_bytes;
        std::uint64_t m_idbkey;
        std::uint16_t m_parts_avail;
        std::uint16_t m_parts_total;
        std::uint64_t m_number;
        std::time_t   m_pubdate;
        std::string   m_subject;
        std::string   m_author;
        // non persistent
    private:
        mutable std::uint16_t m_partno;
        mutable std::uint32_t m_hash;
    };

    template<typename T>
    const std::uint32_t article<T>::MAGIC = 0xc0febabe;


    // we can optimize article object for a specific storage type.
    // when the article data is provided by a memory mapped file
    // we can address the article data without copying by accessing
    // the data through the base pointer at the correct offsets.
    // this will avoid *all* copying of article data.
    template<>
    class article<filemap>
    {
    public:
        article() : m_ptr(nullptr), m_len(0)
        {}

        void load(std::size_t offset, filemap& storage)
        {
            // get a pointer and then cast.
            const auto size  = storage.size();
            const auto avail = size - offset;
            const auto bytes = std::min<std::size_t>(avail, 1024);

            void* base = storage.map_ptr(offset, bytes);

            m_ptr = static_cast<data*>(base);
            m_len = sizeof(data);

            std::uint16_t len_subject;
            std::memcpy(&len_subject, (char*)base + m_len, 2);
            m_len += 2;
            m_subject = str::string_view{(char*)base + m_len,
                len_subject};

            m_len += len_subject;

            std::uint16_t len_author;
            std::memcpy(&len_author, (char*)base + m_len, 2);

            m_len += 2;
            m_author = str::string_view{(char*)base + m_len,
                len_author};

            m_len += len_author;
            m_len += 4; // MAGIC
        }
        void save(std::size_t offset, filemap& storage)
        {
            // nothing to do. writes to the memory address
            // will go to the backend file already.
        }

        str::string_view subject() const
        { return m_subject; }

        std::string subject_as_string() const
        { return m_subject.as_str(); }

        str::string_view author() const
        { return m_author; }

        bitflag<fileflag> bits() const
        { return m_ptr->bits; }

        std::uint32_t bytes() const
        { return m_ptr->bytes; }

        std::uint32_t index() const
        { return m_ptr->index; }

        std::uint32_t num_parts_total() const
        { return m_ptr->parts_total; }

        std::uint32_t num_parts_avail() const
        { return m_ptr->parts_avail; }

        std::uint64_t idbkey() const
        { return m_ptr->idbkey; }

        std::uint64_t number() const
        { return m_ptr->number; }

        std::time_t pubdate() const
        { return m_ptr->pubdate; }

        std::uint32_t size_on_disk() const
        { return m_len; }

        bool test(fileflag flag) const
        {
            bitflag<fileflag> b(m_ptr->bits);
            return b.test(flag);
        }

        bool is_broken() const
        { return test(fileflag::broken); }

        bool is_binary() const
        { return test(fileflag::binary); }

        bool is_deleted() const
        { return test(fileflag::deleted); }

        bool is_bookmarked() const
        { return test(fileflag::bookmarked); }

        bool is_utf8_enabled() const
        { return test(fileflag::enable_utf8); }

        bool has_parts() const
        { return m_ptr->parts_total != 0; }

        filetype type() const
        {
            return (filetype)m_ptr->type;
        }

        void set_bits(fileflag flags, bool bit)
        {
            bitflag<fileflag> f(m_ptr->bits);
            f.set(flags, bit);

            m_ptr->bits = f.value();
        }

    private:
        // important. this struct is binary compatible
        // with raw data written by the non-specialize article template.
        // this must be kept in sync with that code
#if defined(__MSVC__)
#  pragma pack(push, 1)
        struct data {
#else
        struct __attribute__ ((packed)) data {
#endif
            std::uint32_t index;
            std::uint32_t bytes;
            std::uint64_t idbkey;
            std::uint16_t parts_avail;
            std::uint16_t parts_total;
            std::uint64_t number;
            std::time_t   pubdate;
            std::uint8_t  bits;
            std::uint8_t  type;
            //std::uint16_t subject_len;
            //char subject[1];
        };
#if defined(__MSVC__)
#  pragma pack(pop)
#endif

        //static_assert(sizeof(bitflag<fileflag>) == 1, "");
        //static_assert(sizeof(filetype) == 1, "");

        data* m_ptr;

    private:
        str::string_view m_subject;
        str::string_view m_author;
        std::uint32_t m_len;
    };

} // newsflash
