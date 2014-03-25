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

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace engine
{
    struct account;
    struct configuration;
    class listener;

    // engine manages collections of connections
    // and download tasks/items. 
    class bigmachine
    {
    public:
        struct stats {
            std::uint64_t bytes_downloaded;
            std::uint64_t bytes_written;
            std::uint64_t bytes_queued;
        };

        // newsserver content comprising of a list of message(article) ids.
        // and other parameters instructing the engine how to perform the download.
        struct content {
            // list of message id's or message numbers.
            // note that message numbers are specific to a server
            // while message-ids are portable across servers.
            std::vector<std::string> ids;

            // the list of groups into which look for the messge ids.
            std::vector<std::string> groups;

            // the local filesystem path where the content
            // is to the be placed.
            std::string path;

            // the human readable description. 
            // this will appear in task::description.
            std::string description;

            // the esimated size of the content to be downloaded in bytes. 
            std::uint64_t size;

            // the account to be used for downloading the content.
            std::size_t account;
        };

        // create a new engine with the given listener object and logs folder.
        bigmachine(listener& callback, std::string logs);
       ~bigmachine();

        // configure the engine. this is an asynchronous call
        // and returns immediately.
        void configure(const configuration& conf);

        // configure an account in the engine.
        // if the account already exists then the account is updated
        // otherwise it's created.
        // this is an async call and returns immediately.
        void configure(const account& acc);

        // request the engine to start shutdown. when the engine is ready is shutting
        // down a callback is invoked. Then the engine can be deleted.
        // This 2 phase stop allows the GUI to for example animate while 
        // the engine performs an orderly shutdown
        void shutdown();

        // download a single piece of content
        void download(const content& cont);

        // download multiple pieces of content and put them into
        // one single "batch". 
        void download(const std::vector<content>& contents);

    private:
        class impl;

        std::unique_ptr<impl> pimpl_;
    };

} // engine
