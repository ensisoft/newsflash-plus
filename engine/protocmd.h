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

#include <boost/algorithm/string/trim.hpp>
#include <zlib/zlib.h>
#include <algorithm>
#include <functional>
#include <string>
#include <exception>
#include <vector>
#include <deque>
#include <tuple>
#include <sstream>
#include <cstdint>
#include <cassert>

namespace nntp
{
    typedef unsigned int code_t;
    typedef std::initializer_list<code_t> code_list_t;

    const code_t AUTHENTICATION_REQUIRED = 480;
    const code_t AUTHENTICATION_ACCEPTED = 281;
    const code_t PASSWORD_REQUIRED       = 381;

    // adapters for contiguous std types that can be used as a buffer.
    inline 
    size_t buffer_capacity(const std::string& str) 
    { 
        return str.size(); 
    }
    inline 
    void* buffer_data(std::string& str) 
    { 
        return &str[0]; 
    }
    inline 
    void  grow_buffer(std::string& str, size_t capacity) 
    { 
        str.resize(capacity); 
    }

    template<typename T> inline
    size_t buffer_capacity(const std::vector<T>& vec) 
    { 
        return vec.size() * sizeof(T); 
    }

    template<typename T> inline
    void* buffer_data(std::vector<T>& vec) 
    { 
        return &vec[0]; 
    }

    template<typename T> inline
    void grow_buffer(std::vector<T>& vec, size_t capacity) 
    { 
        const size_t items = (capacity + sizeof(T) - 1) / sizeof(T);
        vec.resize(items);
    }

    namespace detail {

        // scan the input buffer for the a complete response.
        // returns the length of the response if found, otherwise 0.
        inline 
        size_t find_response(const void* buff, size_t size)
        {
            // end of response is indicated by \r\n
            if (size < 2)
                return 0;

            const char* str = static_cast<const char*>(buff);            

            for (size_t len=1; len<size; ++len)
            {
                if (str[len] == '\n' && str[len-1] == '\r')
                    return len+1;
            }
            return 0;
        }

        // scan the input buffer for a complete message body.
        // returns the length of the body if found, otherwise 0.
        inline
        size_t find_body(const void* buff, size_t size)
        {
            // this is referred to as "multi-line data block" in rfc3977
            // end of body data is indicated by \r\n.\r\n
            if (size < 5)
                return 0;

            const char* str = static_cast<const char*>(buff);

            for (size_t len=4; len<size; ++len)
            {
                if (str[len] == '\n' && str[len-1] == '\r' && str[len-2] == '.'
                    && str[len-3] == '\n' && str[len-4] == '\r')
                    return len+1;
            }
            return 0;
        }

        struct trailing_comment {
            std::string str;
        };

        template<typename T>
        void extract_value(std::stringstream& ss, T& value)
        {
            ss >> std::skipws >> value;
        }

        inline
        void extract_value(std::stringstream& ss, std::string& str)
        {
            ss >> std::skipws >> str;
        }

        inline
        void extract_value(std::stringstream& ss, trailing_comment& comment)
        {
            std::getline(ss, comment.str);
            boost::algorithm::trim(comment.str);
        }

        template<typename Value>
        void scan_next_value(std::stringstream& ss, Value& next)
        {
            extract_value(ss, next);
        }

        template<typename Value, typename... Rest>
        void scan_next_value(std::stringstream& ss, Value& next, Rest&... gang)
        {
            extract_value(ss, next);
            scan_next_value(ss, gang...);
        }

        template<typename... Values>
        bool scan_response(const std::string& response, Values&... values)
        {
            std::stringstream ss(response);
            scan_next_value(ss, values...);
            return !ss.fail();
        }

        inline
        bool check_code(code_t code, const code_list_t& allowed_codes)
        {
            const auto it = std::find(allowed_codes.begin(), 
                allowed_codes.end(), code);
            return it != allowed_codes.end();
        }

        inline
        std::deque<std::string> split_lines(const std::string& input)
        {
            std::deque<std::string> ret;
            std::stringstream ss(input);
            std::string line;
            while (!ss.eof())
            {
                std::getline(ss, line);
                ret.push_back(line);
            }
            return ret;
        }

    } // detail

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

    struct cmd {
        std::function<size_t (void*, size_t)>     cmd_recv;
        std::function<void (const void*, size_t)> cmd_send;
        std::function<void (const std::string&)>  cmd_log;

        void send(const std::string& cmd)
        {
            std::string str(cmd);
            if (cmd_log)
            {
                if (cmd.find("AUTHINFO PASS") != std::string::npos)
                     cmd_log("AUTHINFO PASS *********");
                else if (cmd.find("AUTHINFO USER") != std::string::npos)
                     cmd_log("AUTHINFO USER *********");
                else cmd_log(str);
            }

            str.append("\r\n");
            cmd_send(str.c_str(), str.size());
        }

        template<typename Pred>
        std::pair<size_t, size_t> recv(Pred test, void* buff, size_t buffsize)
        {
            size_t total_bytes_received  = 0;
            size_t message_length = 0;
            do 
            {
                const size_t capacity = buffsize - total_bytes_received;

                const auto bytes = cmd_recv(((char*)buff) + total_bytes_received, capacity);
                if (bytes == 0)
                    throw nntp::exception("no input");

                total_bytes_received += bytes;
                message_length = test(buff, total_bytes_received);

            } 
            while (!message_length && (total_bytes_received < buffsize));

            return { message_length, total_bytes_received };
        }

        template<typename Pred>
        std::pair<size_t, size_t> recv(Pred test, std::string& buff)
        {
            return recv(test, &buff[0], buff.size());
        }
        
        // perform a send/recv transaction. no body is returned.
        code_t transact(const std::string& cmd, const code_list_t& allowed_codes)
        {
            if (!cmd.empty())
                send(cmd);

            std::string response(512, 0);

            const auto& ret = recv(detail::find_response, response);
            if (!ret.first)
                throw nntp::exception("no response was found within the response buffer");

            response.resize(ret.first-2);
            if (cmd_log)
                cmd_log(response);

            code_t status = 0;
            if (!detail::scan_response(response, status) || 
                !detail::check_code(status, allowed_codes))
                throw nntp::exception("incorrect command response");

            return status;
        }        

        // perform send/recv transaction. read the response body into the body Buffer
        template<typename Buffer>
        std::tuple<code_t, size_t, size_t> transact(const std::string& cmd, 
            const code_list_t& allowed_codes, const code_list_t& success_codes, Buffer& body)
        {
            send(cmd);

            assert(buffer_capacity(body));

            const auto& ret = recv(detail::find_response, buffer_data(body), buffer_capacity(body));
            if (!ret.first)
                throw nntp::exception("no response was found within the response buffer");

            const std::string response((char*)buffer_data(body), ret.first-2);

            if (cmd_log)
                cmd_log(response);

            code_t status = 0;
            if (!detail::scan_response(response, status) ||
                !detail::check_code(status, allowed_codes))
                throw nntp::exception("incorrect command response");

            if (!detail::check_code(status, success_codes))
                return std::make_tuple(status, 0, 0);

            assert(ret.first <= ret.second);

            size_t data_total  = ret.second;
            size_t body_offset = ret.first; 

            // get a pointer to the start of the buffer
            // and skip the response header. 
            char* body_ptr = static_cast<char*>(buffer_data(body));

            // check if we've already have the full body in the buffer
            size_t body_len = detail::find_body(body_ptr + body_offset, data_total - body_offset);

            while (!body_len)
            {
                const size_t capacity = buffer_capacity(body);
                size_t available = capacity - data_total;
                if (!available)
                {
                    available = capacity;
                    grow_buffer(body, capacity * 2);
                    body_ptr = static_cast<char*>(buffer_data(body));
                }
                const auto& ret = recv(detail::find_body, body_ptr + data_total, available);

                data_total += ret.second;
                body_len    = ret.first;
            }

            assert(body_len >= 5);
            assert(data_total >= body_len);
            
            return std::make_tuple(status, body_offset, data_total - body_offset - 5);  // omit \r\n.\r\n from the body length
        }

    };


    // receive initial greeting from the server.
    // 200 Service available, posting allowed
    // 201 Service available, posting prohibited
    // 400 Service temporarily unavailable
    // 502 Service permanently unavailable
    struct cmd_welcome : cmd {
        code_t transact() 
        {
            // we don't send anything here. just receive response.
            return cmd::transact("", {200, 201, 400, 502});
        }
        enum : code_t { 
            SERVICE_TEMPORARILY_UNAVAILABLE = 400,
            SERVICE_PERMANENTLY_UNAVAILABLE = 502
        };
    };

    // send session terminating QUIT command. 
    // the server will respond with 205 and then close
    // the connection.
    // 205 goodbye
    struct cmd_quit : cmd {
        code_t transact()
        {
            return cmd::transact("QUIT", {205});
        }
    };

    // once this is enabled it will stay on
    // untill connection is disconnected and restarted.
    // 222 ok
    struct cmd_enable_compress_gzip : cmd {
        code_t transact()
        {
            return cmd::transact("XFEATURE COMPRESS GZIP", {222});
        }
        enum : code_t { SUCCESS = 222 };
    };

    // if the server is mode switching request it to switch
    // from transit mode to it's reader mode. 
    // this command must not be pipelined.
    // 200 posting allowed
    // 201 posting prohibited
    // 502 reading service permanently unavailable
    struct cmd_mode_reader : cmd {
        code_t transact() 
        {
            return cmd::transact("MODE READER", {200, 201, 502, 480});
        }
    };

    // send AUTHINFO USER to authenticate the current session.
    // this should only be sent when 480 response requesting
    // authentication is received.
    // 281 authentication accepted
    // 381 password required
    // 481 authentication failed/rejected
    // 482 authentication command issued out of sequence
    // 502 command unavailable.
    struct cmd_auth_user : cmd {
        const std::string user;

        cmd_auth_user(std::string username) : user(std::move(username))
        {}

        code_t transact() 
        {
            return cmd::transact("AUTHINFO USER " + user, {281, 381, 481, 482, 502});
        }
    };

    // send AUTHINFO PASS to authenticate the current session.
    // should only be sent when 318 password required is received.
    // 281 authentication accepted
    // 381 password required
    // 481 authentication failed/rejected
    // 482 authentication command issued out of sequence
    // 502 command unavailable 
    struct cmd_auth_pass : cmd {
        const std::string pass;

        cmd_auth_pass(std::string password) : pass(std::move(password))
        {}

        code_t transact()
        {
            return cmd::transact("AUTHINFO PASS " + pass, {281, 381, 481, 482, 502});
        }
    };

    // request the server to select the current newsgroup to the 
    // given group. succesful selection will return the first and
    // last article numbers in the group and an estimate of total
    // articles in the group.
    // 211 number low high groupname Group succesfully selected
    // 411 no such newsgroup
    struct cmd_group : cmd {
        const std::string group;

        uint64_t count;
        uint64_t low;
        uint64_t high;

        cmd_group(std::string groupname) : 
            group(std::move(groupname)), count(0), low(0), high(0)
        {}

        code_t transact()
        {
            send("GROUP " + group);

            std::string response(512, 0);

            const auto& ret = recv(detail::find_response, response);
            if (!ret.first)
                throw nntp::exception("no response found");

            response.resize(ret.first-2);

            if (cmd_log)
                cmd_log(response);

            code_t code = 0;
            const bool success = detail::scan_response(response, code, count, low, high);
            if (((code != 211) && (code != 411)) || (code == 211 && !success))
                throw exception("incorrect command response");

            return code;
        }
        enum : code_t { SUCCESS = 211 };      
    };


    // request the server for the current list of capabilities. 
    // this mechanism includes some standard capabilities as well as server
    // specific extensions. upon receiving a response we scan
    // the list of returned capabilities and set the appropriate boolean flags
    // to indicate the existence of said capability.
    // it seems that some servers such as astraweb respond to this with 500
    // 101 capability list follows.
    // 480 authentication required
    // 500 what? 
    struct cmd_capabilities : cmd {

        enum : code_t { SUCCESS = 101 };

        bool has_mode_reader;
        bool has_compress_gzip;
        bool has_xzver;

        cmd_capabilities() : has_mode_reader(false), has_compress_gzip(false), has_xzver(false)
        {}

        code_t transact()
        {
            std::string response(1024, 0);

            size_t offset = 0;
            size_t size   = 0;
            code_t code   = 0;

            std::tie(code, offset, size) = cmd::transact("CAPABILITIES", {101, 480, 500}, {101}, response);
            if (code == SUCCESS)
            {
                const auto& lines = detail::split_lines(response);

                for (size_t i=1; i<lines.size(); ++i)
                {
                    const auto& cap = lines[i];
                    if (cap.find("MODE-READER") != std::string::npos)
                        has_mode_reader = true;
                    else if (cap.find("XZVER") != std::string::npos)
                        has_xzver = true;
                    else if (cap.find("COMPRESS") != std::string::npos && cap.find("GZIP") != std::string::npos)
                        has_compress_gzip = true;
                }
            }
            return code;
        }
    };

    // request a list of valid newsgroups and associated information.
    // 215 list of newsgroups follows.
    template<typename Buffer>
    struct cmd_list : cmd {

        enum : code_t { SUCCESS = 215 };

        Buffer& buffer;
        size_t offset;
        size_t size;

        cmd_list(Buffer& buff) : buffer(buff)
        {}

        code_t transact()
        {
            code_t status = 0;
            std::tie(status, offset, size) = cmd::transact("LIST", {215}, {215}, buffer);

            return status;
        }
    };

    // request to receive the body of the article identified
    // by article. note that this can either be the message number
    // or the message-id in angle brackets ("<" and ">")
    // 222 body follows
    // 420 no article with that message-id
    // 423 no article with that number
    // 412 no newsgroup selected
    template<typename Buffer>
    struct cmd_body : cmd {

        enum : code_t { SUCCESS = 222 };        

        Buffer& buffer;
        const std::string article;        
        size_t offset;
        size_t size;
        std::string reason;

        cmd_body(Buffer& buff, std::string id) : buffer(buff), article(std::move(id)),
            offset(0), size(0)
        {}

        code_t transact()
        {
            code_t status = 0;
            std::tie(status, offset, size) = cmd::transact("BODY " + article, {222, 420, 423, 412}, {222}, buffer);
            if (status != SUCCESS)
            {
                const char* data = static_cast<const char*>(buffer_data(buffer));
                const auto size  = buffer_capacity(buffer);

                const auto len = detail::find_response(data, size);
                assert(len);

                const std::string response(data, len);
                detail::trailing_comment comment;
                detail::scan_response(response, status, comment);
                reason = comment.str;
            }
            return status;
        }
    };

    // like XOVER except that compressed.
    template<typename Buffer>
    struct cmd_xzver : cmd
    {
        enum : code_t { SUCCESS = 224 };

        Buffer& buffer;
        const std::string first;
        const std::string last;
        size_t offset;
        size_t size;        

        cmd_xzver(Buffer& buff, std::string start, std::string end) : buffer(buff),
           first(std::move(start)), last(std::move(end)),
           offset(0), size(0)
        {}

        code_t transact()
        {
            std::vector<char> body(1024 * 1024);

            const auto& ret = cmd::recv(detail::find_response, buffer_data(body), buffer_capacity(body));
            if (!ret.second)
                throw nntp::exception("no response was found within the buffer");

            const std::string response((char*)buffer_data(body), ret.first);
            code_t status = 0;
            if (!detail::scan_response(response, status) ||
                !detail::check_code(status, {224, 412, 420, 502}))
                throw nntp::exception("incorrect command response");

            const size_t body_offset = ret.first;
            const size_t data_total  = ret.second;

            struct Z {
                z_stream z;
                Z() {
                    z = z_stream {0};
                }

               ~Z() {
                    inflateEnd(&z);
                }
            } zstream;


            // begin body decompression, read data utill z inflate is happy.
            zstream.z.avail_in = data_total - body_offset;
            zstream.z.next_in  = (Bytef*)buffer_data(body);
            inflateInit(&zstream.z);

            // uLong out = 0;
            // uLong in  = 0;

            int err = Z_OK;
            do
            {
                zstream.z.next_out = (Bytef*)buffer_data(buffer) + size;
                zstream.z.avail_out = buffer_capacity(buffer) - size;
                zstream.z.next_in = (Bytef*)buffer_data(body);
                zstream.z.avail_in  = 0;
            }
            while (err != Z_STREAM_END);

            return status;
        }
    };

    // XOVER is part of common nntp extensions
    // https://www.ietf.org/rfc/rfc2980.txt 
    // and is superceded by RFC 3977 and OVER
    // 224 overview information follows
    // 412 no news group selected
    // 420 no articles(s) selected
    // 502 no permission    
    template<typename Buffer>
    struct cmd_xover : cmd
    {
        enum : code_t { SUCCESS = 224 };            
    
        Buffer& buffer;
        const std::string first;
        const std::string last;
        size_t offset;
        size_t size;

        cmd_xover(Buffer& buff, std::string start, std::string end) : buffer(buff),
             first(std::move(start)), last(std::move(end)),
             offset(0), size(0)
        {}

        code_t transact()
        {
            code_t status = 0;
            std::tie(status, offset, size) = cmd::transact("XOVER " + first + "-" + last, {224, 412, 420, 502}, {224}, buffer);                

            return status;
        }
    };

} // nntp
