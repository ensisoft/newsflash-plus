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

#include <zlib/zlib.h>
#include <algorithm>
#include <functional>
#include <string>
#include <exception>
#include <vector>
#include <deque>
#include <sstream>
#include <cstdint>

namespace nntp
{
    const int AUTHENTICATION_REQUIRED = 480;
    const int AUTHENTICATION_ACCEPTED = 281;
    const int PASSWORD_REQUIRED       = 381;
    const int SERVICE_UNAVAILABLE     = 502;

    namespace detail {
        inline 
        bool find_response(const std::string& str)
        {
            // end of response is indicated by \r\n
            const auto end = str.size();
            if (end < 2)
                return false;
            bool found_end = (str[end-1] == '\n' && str[end-2] == '\r');

            return found_end;
        }

        inline
        bool find_body(const std::string& str)
        {
            // end of body data is indicated by .\r\n
            const auto end = str.size();
            if (end < 3)
                return false;

            bool found_end = (str[end-1] == '\n' &&
                    str[end-2] == '\r' &&
                    str[end-3] == '.');
           
            return found_end;
        }
        inline
        bool find_body_buff(const char* buff, size_t len)
        {
            if (len < 3)
                return false;
                
            bool found_end = (buff[len-3] == '\n' &&
                              buff[len-2] == '\r' &&
                              buff[len-1] == '.');
            return found_end;
        }


        template<typename T>
        void extract_value(std::stringstream& ss, T& value)
        {
            ss >> std::skipws >> value;
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
        bool check_code(int code, std::initializer_list<int> allowed_codes)
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
        exception(std::string what) noexcept 
           : what_(std::move(what))
        {}
        const char* what() const noexcept
        {
            return what_.c_str();
        }
    private:
        const std::string what_;
    };

    struct cmd 
    {
        std::function<size_t (void*, size_t)>     cmd_recv;
        std::function<void (const void*, size_t)> cmd_send;
        std::function<void (const std::string&)>  cmd_log;

        void send(const char* cmd)
        {
            std::string str(cmd);
            cmd_log(str);

            str.append("\r\n");
            cmd_send(str.c_str(), str.size());
        }
        void send(const std::string& cmd)
        {
            std::string str(cmd);
            cmd_log(str);

            str.append("\r\n");
            cmd_send(str.c_str(), str.size());
        }

        template<typename Pred>
        std::string recv(Pred test)
        {
            std::string buff(512, 0);
            std::string ret;
            do 
            {
                if (ret.size() >= 1024)
                    throw nntp::exception("message is too large");
                const auto bytes = cmd_recv(&buff[0], buff.size());
                if (bytes == 0)
                    throw nntp::exception("no input");

                buff.resize(bytes);
                if (ret.empty())
                    std::swap(buff, ret);
                else ret.append(buff);

            } while (!test(ret));

            return ret;
        }
        
        template<typename Pred>
        size_t recv(Pred test, char* buff, size_t capacity)
        {
            size_t bytes = 0;
            do
            {
                if (bytes == capacity)
                    throw nntp::exception("message is too large");
                const auto ret = cmd_recv(buff + bytes, capacity - bytes);
                if (ret == 0)
                    throw nntp::exception("no input");
                bytes += ret;
            }
            while (!test(buff, bytes));
            
            return bytes;
        }
    };

    // receive initial 200 Welcome from the server
    struct cmd_welcome : cmd 
    {
        int transact() 
        {
            // we don't send anything but only expect to receive the greeting.
            
            // valid responses:
            // 200 Service available, posting allowed
            // 201 Service available, posting prohibited
            // 400 Service temporarily unavailable
            // 502 Service permanently unavailable
            const auto& response = recv(detail::find_response);

            int code = 0;
            if (!detail::scan_response(response, code) ||
                !detail::check_code(code, {200, 201, 400, 502}))
                throw exception("incorrect greeting received from the server");

            return code;
        }
    };

    struct cmd_capabilities : cmd
    {
        static const int SUCCESS = 101;

        cmd_capabilities() : 
            has_compress_gzip(false), has_xzver(false), has_mode_reader(false)
        {}

        int transact()
        {
            send("CAPABILITIES");

            // the capabilities is sent as a list one caps per line
            // terminated by an empty line, i.e. .\r\n
            // read input data untill empty line is received
            // valid responses:
            // 101 capability list follows (multi-line)\r\n
            // IHAVE\r\n
            // MODE-READER\r\n
            // ...
            // .\r\n
            const auto& response = recv(detail::find_body);

            auto lines = detail::split_lines(response);

            int code = 0;
            if (!detail::scan_response(lines.at(0), code) ||
                !detail::check_code(code, {101}))
                throw nntp::exception("incorrect CAPABILITIES response");

            // remove the initial-response-line
            // then the rest of the lines contain capabilities
            lines.pop_front();

            for  (const auto& cap : lines)
            {
                if (cap.find("MODE-READER") != std::string::npos)
                    has_mode_reader = true;
                else if (cap.find("XZVER") != std::string::npos)
                    has_xzver = true;
                else if (cap.find("COMPRESS") != std::string::npos && cap.find("GZIP") != std::string::npos)
                    has_compress_gzip = true;
            }
            return code;
        }

        bool has_compress_gzip;
        bool has_xzver;
        bool has_mode_reader;
    };

    struct cmd_mode_reader : cmd
    {
        int transact() 
        {
            send("MODE READER");

            // valid responses.
            // 200 posting allowed
            // 201 posting prohibited
            // 502 reading service permanently unavailable
            const auto& response = recv(detail::find_response);

            int code = 0;
            if (!detail::scan_response(response, code) ||
                !detail::check_code(code, {200, 201, 502}))
                throw nntp::exception("incorrect MODE READER response");

            return code;
        }
    };

    // send AUTHINFO USER
    struct cmd_auth_user : cmd 
    {
        const std::string user;

        cmd_auth_user(std::string username) : user(std::move(username))
        {}

        int transact() 
        {
            send("AUTHINFO USER " + user);

            // valid responses.
            // 281 Authentication accepted
            // 381 Password Required
            // 481 Authentication failed/rejected
            // 482 Authentication command issued out of sequence
            // 502 Command unavailable
            const auto& response = recv(detail::find_response);

            int code = 0;
            if (!detail::scan_response(response, code) ||
                !detail::check_code(code, {281, 381, 481, 482, 502}))
                throw nntp::exception("incorrect AUTHINFO USER response");

            return code;
        }
    };

    // send AUTHINFO PASS
    struct cmd_auth_pass : cmd 
    {
        const std::string pass;

        cmd_auth_pass(std::string password) : pass(std::move(password))
        {}

        int transact()
        {
            cmd_log("AUTHINFO PASS ********");

            std::string str("AUTHINFO PASS ");
            str.append(pass);

            str.append("\r\n");
            cmd_send(str.c_str(), str.size());

            // valid responses:
            // 281 Authentication accepted
            // 381 Password Required
            // 481 Authentication failed/rejected
            // 482 Authentication command issued out of sequence
            // 502 Command unavailable
            const auto& response = recv(detail::find_response);            
            
            int code = 0;
            if (!detail::scan_response(response, code) ||
                !detail::check_code(code, {281, 381, 481, 482, 502}))
                throw nntp::exception("incorrect AUTHINFO PASS response");

            return code;
        }
    };

    struct cmd_group : cmd 
    {
        const static int SUCCESS = 211;

        const std::string group;

        cmd_group(std::string groupname) : group(std::move(groupname)),
            article_count(0), high_water_mark(0), low_water_mark(0)
        {}

        int transact()
        {
            send("GROUP " + group);

            // valid responses:
            // 211 number low high group  Group succesfully selected
            // 411 No such newsgroup
            const auto& response = recv(detail::find_response);

            int code = 0;
            bool scan = detail::scan_response(response, code, article_count, low_water_mark, high_water_mark);         
            if (((code != 211) && (code != 411)) || (code == 211 && !scan))
                throw nntp::exception("incorrect GROUP response");

            return code;
        }

        uint64_t article_count;
        uint64_t high_water_mark;
        uint64_t low_water_mark;
    };


    struct cmd_list : cmd
    {
        int transact()
        {
            send("LIST");

            return 0;
        }
    };

    struct cmd_body : cmd
    {
        const static int SUCCESS = 222;

        const std::string article;
        const size_t capacity;
        char* buff;
        size_t offset;
        size_t size;        

        cmd_body(std::string articleid, size_t buffcapa, char* ptr) 
            : article(std::move(articleid)), capacity(buffcapa), buff(ptr),
              offset(0), size(0)
        {}

        int transact()
        {
            send("BODY " + article);

            // valid responses:
            // 222 0|n messageid Body follows
            // 222 n messageid body follows
            // 430 no article with that message-id
            // 423 no article with that number
            // 412 no newsgroup selected.
            const size_t len = recv(detail::find_body_buff, buff, capacity);
            
            std::string response;
            // extract the response line
            for (size_t i=0; i<len; ++i)
            {
                response.push_back(buff[i]);
                if (i > 2)
                {
                    if (response[i] == '\n' && response[i-1] == '\r')
                       break;
                }                
            }
            
            int code = 0;
            if (!detail::scan_response(response, code) ||
                !detail::check_code(code, {222, 430, 423, 412}))
                throw nntp::exception("incorrect BODY response");
            
            if (code == 222)
            {
                offset = response.size();
                size   = len - 3 - response.size();
            }

            return code;
        }

    };

    // like XOVER except that compressed.
    struct cmd_xzver : cmd
    {
        const std::string first;
        const std::string last;
        const size_t capacity;
        char* buff;
        size_t offset;
        size_t size;        

        cmd_xzver(std::string f, std::string l, size_t buffcapa, char* ptr)
            : first(std::move(f)), last(std::move(l)), capacity(buffcapa), buff(ptr)
        {}

        int transact()
        {
            std::stringstream ss;
            ss << "XZVER " << first << "-" << last;
            send(ss.str());

            // 224 overview information follows
            // 412 no news group selected
            // 420 no articles(s) selected
            // 502 no permission
            // const size_t len = recv(detail::find_body_buff, buff, capacity);

            // // extract response line
            // std::string response;
            // for (size_t i=0; i<len; ++i)
            // {
            //     response.push_back(buff[i]);
            //     if (i > 2)
            //     {
            //         if (response[i] == '\n' && response[i-1] == '\r')
            //             break;
            //     }
            // }
            // int code = 0;
            // if (!detail::scan_response(response, code) ||
            //     !detail::check_code(code, {224, 412, 420, 502}))
            //     throw nntp::exception("incorrect XOVER response");

            // if (code == 224)
            // {
            //     offset = response.size();
            //     size   = len - 3 - response.size(); 
            // }
            // return code;
            return 0;
        }
    };

    // XOVER is part of common nntp extensions
    // https://www.ietf.org/rfc/rfc2980.txt 
    // and is superceded by RFC 3977 and OVER
    struct cmd_xover : cmd
    {
        const std::string first;
        const std::string last;
        const size_t capacity;
        char* buff;
        size_t offset;
        size_t size;

        cmd_xover(std::string f, std::string l, size_t buffcapa, char* ptr)
            : first(std::move(f)), last(std::move(l)), capacity(buffcapa), buff(ptr)
        {}

        int transact()
        {
            std::stringstream ss;
            ss << "XOVER " << first << "-" << last;
            send(ss.str());

            // 224 overview information follows
            // 412 no news group selected
            // 420 no articles(s) selected
            // 502 no permission
            const size_t len = recv(detail::find_body_buff, buff, capacity);

            // extract response line
            std::string response;
            for (size_t i=0; i<len; ++i)
            {
                response.push_back(buff[i]);
                if (i > 2)
                {
                    if (response[i] == '\n' && response[i-1] == '\r')
                        break;
                }
            }
            int code = 0;
            if (!detail::scan_response(response, code) ||
                !detail::check_code(code, {224, 412, 420, 502}))
                throw nntp::exception("incorrect XOVER response");

            if (code == 224)
            {
                offset = response.size();
                size   = len - 3 - response.size(); 
            }
            return code;
        }
    };


    // once this is enabled it will stay on
    // untill connection is disconnected and restarted.
    struct cmd_enable_compress_gzip : cmd
    {
        const static int SUCCESS =  222;

        int transact()
        {
            send("XFEATURE COMPRESS GZIP");

            const auto& response = recv(detail::find_response);

            int code = 0;
            if (!detail::scan_response(response, code) ||
                !detail::check_code(code, {222}))
                throw nntp::exception("incorrect XFEATURE COMPRESS GZIP response");

            return code;
        }

    };

} // nntp
