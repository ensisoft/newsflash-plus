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

// $Id: socket.h,v 1.11 2010/04/07 12:57:18 svaisane Exp $

#pragma once

#include <newsflash/config.h>
#include "socketapi.h"
#include "waithandle.h"

namespace newsflash
{
    // socket represents a low level socket connection to the remote server
    // for sending and receiving data
    class socket 
    {
    public:
        virtual ~socket() = default;
        // Connect this socket to the specified host on the specified port.
        // This function is non-blocking and returns immediately.
        // To complete the connection attempt call complete connect
        // after the waithandle becomes signaled.
        virtual void begin_connect(ipv4addr_t host, ipv4port_t port) = 0;

        // Complete the connection attempt. On succesful return
        // the connection is ready to be used for sending and receiving data.
        // on error an exception is thrown.
        virtual void complete_connect() = 0;
        
        // Write all of the input data to the socket.
        // On error an exception is thrown.
        virtual void sendall(const void* buff, int len) = 0;

        // Write some input data to the socket.
        // Returns numbers of bytes written.
        // on error an exception is thrown.
        virtual int sendsome(const void* buff, int len) = 0;

        // Receive some data into the buffer.
        // Returns the number of bytes received (which can be 0).
        // on error an exception is thrown.
        virtual int recvsome(void* buff, int capacity) = 0;

        // Close the socket.
        virtual void close() = 0;

        // Get handle for waiting on the socket for writability/readability.
        virtual waithandle wait() const = 0;

        // Get handle for waiting either writability or readability or both.
        virtual waithandle wait(bool waitread, bool waitwrite) const = 0;

        // returns true if the socket buffer has more data for immediate read.
        virtual bool can_recv() const = 0;
    protected:
    private:
    };

} // newsflash

