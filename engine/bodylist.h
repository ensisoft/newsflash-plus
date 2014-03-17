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

#include <functional>
#include <deque>
#include <string>
#include <memory>
#include <mutex>
#include "cmdlist.h"

namespace newsflash
{
    class buffer;

    // list of article numbers (or message-ids) to download.
    class bodylist : public cmdlist
    {
    public:
        // article status
        enum class status {
            unavailable, dmca, success
        };

        // article body response
        struct body {
            std::string article;
            std::shared_ptr<buffer> buff;
            std::size_t id;
            bodylist::status status;
        };

        // callback to be invoked on each body in the list.
        std::function<void (const bodylist::body& body)> on_body;

        bodylist(std::deque<std::string> groups,
                 std::deque<std::string> articles);

       ~bodylist();

        virtual bool run(protocol& proto) override;

    private:
        bool dequeue(std::string& article, std::size_t& id);

    private:
        const std::deque<std::string> groups_;

    private:
        std::mutex mutex_;
        std::deque<std::string> articles_;
        std::size_t id_;
    };

} // newsflash
