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
#include <memory>
#include <cstdint>
#include "task.h"

namespace newsflash
{
    class update : public task
    {
    public:
        update(std::string path, std::string group);
       ~update();

        virtual std::shared_ptr<cmdlist> create_commands() override;

        virtual void cancel() override;
        virtual void commit() override;

        virtual void complete(cmdlist& cmd, 
            std::vector<std::unique_ptr<action>>& next) override;

        virtual void complete(action& a, 
            std::vector<std::unique_ptr<action>>& next) override;

        virtual bool has_commands() const override;
    private:
        class parse;
        class store;
        struct state;

    private:
        std::shared_ptr<state> state_;
        std::uint64_t remote_last_;
        std::uint64_t remote_first_;
        std::uint64_t local_last_;
        std::uint64_t local_first_;
        std::uint64_t xover_last_;
        std::uint64_t xover_first_;
    };

} // newsflash
