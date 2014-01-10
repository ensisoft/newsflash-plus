// Copyright (c) 2013, 2014 Sami Väisänen, Ensisoft 
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

// $Id: uuencode.h,v 1.11 2007/12/04 23:54:45 enska Exp $

#if defined(_MSC_VER)
#  undef MIN
#  undef MAX
#  undef min
#  undef max
#endif

#include <cstdint>
#include <cassert>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>

// UUencoding. used mostly to encode images
// http://en.wikipedia.org/wiki/Uuencoding
namespace uuencode
{
    typedef uint8_t u8;

    namespace detail {
        template<typename OutputIter>
        inline void newline(OutputIter& out)
        {
#ifdef _WIN32
            *out++ = '\r';
#endif
            *out++ = '\n';
        }
        inline void newline(std::ostream_iterator<char>& out)
        {
            *out++ = '\n';
        }
    
        inline void newline(std::ostreambuf_iterator<char>& out)
        {
#ifdef _WIN32
            *out++ = '\r';
#endif
            *out++ = '\n';
        }

        inline int encode(int i)
        {
            assert(i < 64);
            // some mailers have problems with space so use
            // grave accent instead
            if(i == 0)
                return '`';
            return i + 32;
        }
        inline char decode(int i)
        {
            // officially grave accent is for NUL padding,
            // but we work around broken decoders that skip the NUL and emit \r or \n instead            
            if (i == '`' || i == '\r' || i == '\n')
                return 0;
            return i - 32;
        }

        template<typename OutputIterator> 
        void encode(uint32_t value, OutputIterator out)
        {
            *out++ = encode((value >> 18) & 0x3f);
            *out++ = encode((value >> 12) & 0x3f);
            *out++ = encode((value >> 6) & 0x3f);
            *out++ = encode((value & 0x3f));
        }

        template<typename OutputIterator>
        void decode(uint32_t value, OutputIterator out, int bytes)
        {
            for (int i=0; i<bytes; ++i)
                *out++ = u8(value >> (2-i)*8);
        }


        template<typename OutputIterator>
        void write(std::vector<u8>& data, OutputIterator out, int nbytes, bool write_newline)
        {                        
            // write length character identifying the number of *data* bytes
            *out++ = encode(nbytes);
            
            // write the encoded data            
            std::copy(data.begin(), data.end(), out);
                     
            // emit newline if needed
            if (write_newline)
                newline(out);
                
            data.clear();                
        }

        template<typename InputIter>
        std::string read_up_untill_whitespace(InputIter& beg, InputIter& end)
        {
            std::string s;
            while (beg != end)
            {
                char c = *beg++;
                if (c == ' ')
                    return s;
                s.push_back(c);
            }
            return s;
        }

        template<typename InputIter>
        std::string read_up_untill_newline(InputIter& beg, InputIter& end)
        {
            std::string s;
            while (beg != end)
            {
                char c = *beg++;
                if (c == '\n')
                    return s;
                if (c != '\r')
                    s.push_back(c);
            }
            return s;
        }

    } // detail

    template<typename OutputIterator>
    void begin_encoding(const std::string& mode, const std::string& file, OutputIterator dest)
    {
        std::stringstream ss;
        std::string s;
        ss << "begin " << mode << " " << file; //<< "\n";
        s = ss.str();
        std::copy(s.begin(), s.end(), dest);
        detail::newline(dest);
    }

    template<typename OutputIterator>
    void end_encoding(OutputIterator dest)
    {
        detail::newline(dest);
        *dest++ = '`';
        detail::newline(dest);
        *dest++ = 'e';
        *dest++ = 'n';
        *dest++ = 'd';
        detail::newline(dest);
    }


    template<typename InputIterator, typename OutputIterator>
    void encode(InputIterator beg, InputIterator end, OutputIterator out)
    {
        // each output line is length character prefixed and all lines
        // except the last line should contain 60 encoded bytes + length character
        std::vector<u8> buff;

        bool encoding_complete = true;

        uint32_t value  = 0;
        uint32_t nbytes = 0;

        while (beg != end)
        {
            encoding_complete = false;

#define UUENCODE_ENCODE_CONCATENATE(shift) \
            if (beg == end) \
                break; \
            value |= ((u8(*beg++) << shift)); \
            ++nbytes

            // read and concatenate 24 bits of input
            UUENCODE_ENCODE_CONCATENATE(16);
            UUENCODE_ENCODE_CONCATENATE(8);
            UUENCODE_ENCODE_CONCATENATE(0);

#undef UUENCODE_ENCODE_CONCATENATE

            detail::encode(value, std::back_inserter(buff));

            value = 0;

            if (nbytes == 45)
            {
                detail::write(buff, out, 45, true);        
                nbytes = 0;
            }
            encoding_complete = true;
        }

        if (encoding_complete)
            return;    
           
        detail::encode(value, std::back_inserter(buff));           
        detail::write(buff, out, nbytes, false);
        
    }

    struct header {
        std::string file;
        std::string mode;
    };

    // read the input and parse the UU header from the input stream
    template<typename InputIter>
    std::pair<bool, header> parse_begin(InputIter& beg, InputIter end)
    {
        header h;
        
        std::string str = detail::read_up_untill_whitespace(beg, end);
        if (str != "begin")
            return {false, h};

        h.mode = detail::read_up_untill_whitespace(beg, end);
        h.file = detail::read_up_untill_newline(beg, end);
        return { true, h };
    }
    

    // parse and consume the UU trailer from the input stream
    template<typename InputIter>
    bool parse_end(InputIter& beg, InputIter end)
    {
#define UUENCODE_PARSE_END(x) \
        if (beg == end || *beg++ != x) \
            return false;

#define UUENCODE_PARSE_END_OPT(x) \
        if (*beg == x) \
            ++beg;

        UUENCODE_PARSE_END('`');
        UUENCODE_PARSE_END_OPT('\r');
        UUENCODE_PARSE_END('\n');
        UUENCODE_PARSE_END('e');
        UUENCODE_PARSE_END('n');
        UUENCODE_PARSE_END('d');

#undef UUENCODE_PARSE_END

        return true;
    }

    // decode input from the input iterator. returns true
    // if decoding was succesfully completed returns true, otherwise false
    template<typename InputIterator, typename OutputIterator>
    bool decode(InputIterator& beg, InputIterator end, OutputIterator out)
    {
        while (beg != end)
        {
            const u8 length = *beg;
            if (length == '`' || length == ' ') // end of uu data
                return true;            
            else if (length == '\r' || length == '\n') // discard line delimeters
            {
                ++beg;
                // two newlines (or NLCR) means incorrect data
                if (length == '\n' && (*beg == '\r' || *beg == '\n'))
                    return false;
                continue;
            }
            else if ((length < 1 + 32) || (length > 45+32)) // check for crap data
                return false;

            // # of data bytes this line should have
            const int data_bytes = length - 32;

            // how many data 8bit triplets (the last one might not be full)
            const int triplets = (data_bytes + 2) / 3;

            // skip length marker
            ++beg;

            // read and decode
            for (int triplet=0; triplet<triplets; ++triplet)
            {
                uint32_t value = 0;

#define UUENCODE_DECODE_CONCATENATE(shift) \
                if (beg == end) \
                    return false; \
                value |= (detail::decode(*beg++) << shift); \

                UUENCODE_DECODE_CONCATENATE(18);
                UUENCODE_DECODE_CONCATENATE(12);
                UUENCODE_DECODE_CONCATENATE(6);
                UUENCODE_DECODE_CONCATENATE(0);

#undef UUENCODE_DECODE_CONCATENATE

                const int remaining = data_bytes - (triplet * 3);

                detail::decode(value, out, std::min(remaining, 3));
            }
        }   
        return false;
    }
  
} // uuencode
