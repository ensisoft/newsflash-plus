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

#include <exception>
#include <chrono>
#include <string>
#include <cstdint>
#include "types.h"
#include "waithandle.h"

namespace newsflash
{
    // socket represents a low level socket connection to the remote server
    // for sending and receiving data
    class socket 
    {
    public:
        // thrown if things go wrong on socket I/O
        class io_exception : public std::exception
        {
        public:
            io_exception(std::string what, native_errcode_t code) NOTHROW // noexcept
               : what_(std::move(what)), code_(code)
            {}
            const char* what() const NOTHROW // noexcept
            {
                return what_.c_str();
            }
            native_errcode_t code() const NOTHROW // noexcept 
            {
                return code_;
            }
        private:
            const std::string what_;
            const native_errcode_t code_;
        };

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
        virtual void connect(ipv4_addr_t host, uint16_t port) = 0;

        // Complete the connection attempt. On succesful return
        // the connection is ready to be used for sending and receiving data.
        // on error a native platform specific error code is returned.
        virtual native_errcode_t complete_connect() = 0;
        
        // Write all of the input data to the socket.
        // On error a socket_io_error is thrown.
        virtual void sendall(const void* buff, int len) = 0;

        // Write some input data to the socket.
        // returns numbers of bytes written.
        // on error a socket_io_error is thrown.
        virtual int sendsome(const void* buff, int len) = 0;

        // Receive some data into the buffer or timeout.
        // if there is no data available within "timeout" milliseconds timeout_occurred
        // is set to true and function returns 0. Otherwise the number of bytes received is returned.
        // On error a socket_io_error is thrown.
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

