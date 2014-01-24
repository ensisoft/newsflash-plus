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

#include <boost/noncopyable.hpp>
#include <memory>
#include <string>
#include <cstdint>

namespace newsflash
{
    class listener;

    class engine : boost::noncopyable
    {
    public:
        // remote newsserver configuration.
        struct server {
            std::string hostname;
            int  port;
            bool ipv6;
            bool ssl;
        };

        // newsservice account information
        struct account {
            size_t id;
            std::string username;
            std::string password;
            server secure;
            server plain;
            int maximum_connections;
            bool try_compression;
            bool try_pipelining;
            bool fillserver;
        };

        // engine configuration
        struct configuration {
            bool overwrite_existing_files; 
            bool discard_text_context; 
            bool auto_connect;
            bool auto_remove_complete_tasks;
            bool prefer_ssl;
            bool enable_throttle;
            bool enable_fillserver;
            int  throttle_bps; // bytes per second.
        };

        struct stats {
            uint64_t bytes_downloaded;
            uint64_t bytes_written;
            uint64_t bytes_queued;
        };

        // create a new engine instance. listener is the
        // listener implementation for the engine callbacks.
        // logfolder specifies a location in the filesystem
        // where to store the log files.
        engine(listener& list, const std::string& logfolder);

       ~engine();

        void shutdown();

    private:
        class impl;

        std::unique_ptr<impl> pimpl_;
    };

} // newsflash
