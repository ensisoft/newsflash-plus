// Copyright (c) 2013 Sami Väisänen, Ensisoft 
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

// $Id: socket.h,v 1.11 2010/04/07 12:57:18 svaisane Exp $

#pragma once

#include <newsflash/config.h>

#include <system_error>
#include <exception>
#include <chrono>
#include <string>
#include <cstdint>
#include "socketapi.h"
#include "waithandle.h"

namespace newsflash
{
    // socket represents a low level socket connection to the remote server
    // for sending and receiving data
    class socket 
    {
    public:
        // error in TCP protocol level.
        class tcp_exception : public std::exception
        {
        public:
            tcp_exception(std::string what, std::error_code code) NOTHROW // noexcept
               : what_(std::move(what)), 
                 code_(std::move(code))
            {}
            const char* what() const NOTHROW // noexcept
            {
                return what_.c_str();
            }
            const std::error_code& code() const NOTHROW // noexcept 
            {
                return code_;
            }
        private:
            const std::string what_;
            const std::error_code code_;
        };

        // error in SSL protocol level.
        class ssl_exception : public std::exception
        {
        public:
            ssl_exception(std::string what) : what_(std::move(what))
            {}

        private:
            const std::string what_;
        };


        virtual ~socket() = default;
        // Connect this socket to the specified host on the specified port.
        // This function is non-blocking and returns immediately.
        // To complete the connection attempt call complete connect
        // after the waithandle becomes signaled.
        virtual void begin_connect(ipv4addr_t host, uint16_t port) = 0;

        // Complete the connection attempt. On succesful return
        // the connection is ready to be used for sending and receiving data.
        // On error an exception is thrown.
        virtual void complete_connect() = 0;
        
        // Write all of the input data to the socket.
        // On error an exception is thrown.
        virtual void sendall(const void* buff, int len) = 0;

        // Write some input data to the socket.
        // Returns numbers of bytes written.
        // On error an exception is thrown.
        virtual int sendsome(const void* buff, int len) = 0;

        // Receive some data into the buffer.
        // Returns the number of bytes received (which can be 0).
        // On error an exception is thrown.
        virtual int recvsome(void* buff, int capacity) = 0;

        // Close the socket.
        virtual void close() = 0;

        // Get handle for waiting on the socket for writability/readability.
        virtual waithandle wait() const = 0;

        // Get handle for waiting either writability or readability or both.
        virtual waithandle wait(bool waitread, bool waitwrite) const = 0;
    protected:
    private:
    };

} // newsflash

