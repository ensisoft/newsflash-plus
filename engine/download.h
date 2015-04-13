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

    // extract encoded content from the buffers
    class download : public task
    {
    public:
        download(std::vector<std::string> groups, std::vector<std::string> articles,
            std::string path, std::string name);
       ~download();

        virtual std::shared_ptr<cmdlist> create_commands() override;

        virtual void cancel() override;
        virtual void commit() override;
        virtual void complete(action& act,
            std::vector<std::unique_ptr<action>>& next) override;
        virtual void complete(cmdlist& cmd,
            std::vector<std::unique_ptr<action>>& next) override;
        virtual void configure(const settings& s) override;
        virtual bool has_commands() const override;
        //virtual bool is_ready() const override;
        virtual std::size_t max_num_actions() const override;

        void add_file(std::shared_ptr<datafile> file)
        {
            files_.push_back(file);           
        }

        const 
        std::vector<std::shared_ptr<datafile>>& files() const 
        { return files_; }

        const 
        std::vector<std::string>& groups() const 
        { return groups_; }

        const 
        std::vector<std::string>& articles() const 
        { return articles_; }

        const std::string& path() const 
        { return path_; }
    private:
        std::vector<std::string> groups_;
        std::vector<std::string> articles_;
        std::vector<std::shared_ptr<datafile>> files_;
        std::vector<char> stash_;
        std::string path_;
        std::string name_;
    private:
        bool overwrite_;
        bool discardtext_;
        bool yenc_;
    };

} // newsflash

