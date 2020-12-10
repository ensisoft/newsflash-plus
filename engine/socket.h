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

#include "newsflash/config.h"

#include <system_error>
#include <memory>

#include "socketapi.h"
#include "waithandle.h"

namespace newsflash
{
    // socket represents a low level socket connection to the remote server
    // for sending and receiving data
    class Socket
    {
    public:
        virtual ~Socket() = default;

        // Connect this socket to the specified host on the specified port.
        // This function is non-blocking and returns immediately.
        // To complete the connection attempt call complete connect
        // after the waithandle becomes signaled.
        virtual void BeginConnect(ipv4addr_t host, ipv4port_t port) = 0;

        // Complete the connection attempt. On succesful return
        // the connection is ready to be used for sending and receiving data.
        virtual void CompleteConnect(std::error_code* error) = 0;

        // Write all of the input data to the socket.
        virtual void SendAll(const void* buff, int len, std::error_code* error) = 0;

        // Write some input data to the socket.
        // Returns numbers of bytes written.
        virtual int SendSome(const void* buff, int len, std::error_code* error) = 0;

        // Receive some data into the buffer.
        // Returns the number of bytes received (which can be 0).
        virtual int RecvSome(void* buff, int capacity, std::error_code* error) = 0;

        // Close the socket.
        virtual void Close() = 0;

        // Get handle for waiting on the socket for writability/readability.
        virtual WaitHandle GetWaitHandle() const = 0;

        // Get handle for waiting either writability or readability or both.
        virtual WaitHandle GetWaitHandle(bool read, bool write) const = 0;

        // returns true if the socket buffer has more data for immediate read.
        virtual bool CanRecv() const = 0;
    protected:
    private:
    };

} // newsflash

#ifdef NEWSFLASH_BUILD_SOCKET_LIB
#  ifdef __MSVC__
#    define NEWSFLASH_LIBRARY_CALL __declspec(dllexport)
#  else
#    define NEWSFLASH_LIBRARY_CALL
#  endif
#else
#  ifdef __MSVC__
#    define NEWSFLASH_LIBRARY_CALL __declspec(dllimport)
#  else
#    define NEWSFLASH_LIBRARY_CALL
#  endif
#endif

// We have a problem with libssl being a conflicting dependency between
// our engine and Qt5. Qt5 uses libssl 1.1.x and we're still on libssl 1.0.x.
// These are not binary compatible.
// Having all the symbols available in the engine library will lead to problems
// when these two libraries are loaded in the process.
// In order to workout this problem we take the socket code and build it into
// a library and tell the linker to hide all symbols *except* for
// MakeNewsflashSocket which is public. Then the rest of the engine code
// simply uses this API to create a socket and libssl stays hidden.
// Extern C is just to simplify  specificing the symbol name (no mangling)
// in the linker file
extern "C"  {
    NEWSFLASH_LIBRARY_CALL void* MakeNewsflashSocketUnsafe(bool ssl);
    NEWSFLASH_LIBRARY_CALL const char* GetSocketAllocationFailureString();
} // extern "C"

// wrapper since on MSVS a function with C linkage cannot return
// a c++ object.
inline std::unique_ptr<newsflash::Socket> MakeNewsflashSocket(bool ssl)
{
    if (void* ret = MakeNewsflashSocketUnsafe(ssl))
        return std::unique_ptr<newsflash::Socket>(static_cast<newsflash::Socket*>(ret));

    const std::string err = GetSocketAllocationFailureString();
    throw std::runtime_error("Socket allocation failure:" + err);
}