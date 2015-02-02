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
#include <string>
#include <vector>
#include "task.h"

namespace newsflash
{
    class datafile;
    class decode;
    class bodylist;

    // extract encoded content from the buffers
    class download : public task
    {
    public:
        download(std::vector<std::string> groups, std::vector<std::string> articles,
            std::string path, std::string name);
       ~download();

        virtual std::unique_ptr<cmdlist> create_commands() override;
        virtual std::unique_ptr<action> kill() override;
        virtual std::unique_ptr<action> finalize() override;

        virtual void complete(action& act,
            std::vector<std::unique_ptr<action>>& next) override;

        virtual void complete(cmdlist& cmd,
            std::vector<std::unique_ptr<action>>& next) override;

        virtual void configure(const settings& s) override;

        virtual double completion() const override;

        virtual bool has_commands() const override
        { return !articles_.empty(); }

        const 
        std::vector<std::shared_ptr<datafile>>& files() const 
        { return files_; }

    private:
        std::vector<std::string> groups_;
        std::vector<std::string> articles_;
        std::vector<std::shared_ptr<datafile>> files_;
        std::string path_;
        std::string name_;
        std::size_t num_commands_done_;
        std::size_t num_commands_total_;
        std::size_t num_bytes_done_;
    private:
        bool enable_overwrite_;
        bool enable_discardtext_;
    };

} // newsflash

