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

#pragma once

#include "newsflash/config.h"

#include <memory>
#include <string>
#include <vector>

#include "task.h"

namespace newsflash
{
    class datafile;
    class decode;

    // extract encoded content from the buffers
    class Download : public Task
    {
    public:
        Download(const std::vector<std::string>& groups,
            const std::vector<std::string>& articles,
            const std::string& path,
            const std::string& name);
       ~Download();

        // Task implementation
        virtual std::shared_ptr<CmdList> CreateCommands() override;
        virtual void Cancel() override;
        virtual void Commit() override;
        virtual void Complete(action& act,
            std::vector<std::unique_ptr<action>>& next) override;
        virtual void Complete(CmdList& cmd,
            std::vector<std::unique_ptr<action>>& next) override;
        virtual void Configure(const Settings& settings) override;
        virtual bool HasCommands() const override;
        virtual std::size_t MaxNumActions() const override;

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
        std::shared_ptr<datafile> create_file(const std::string& name, std::size_t assumed_size);

    private:
        using stash = std::vector<char>;

        std::vector<std::string> groups_;
        std::vector<std::string> articles_;
        std::vector<std::shared_ptr<datafile>> files_;
        std::vector<std::unique_ptr<stash>> stash_;
        std::string path_;
        std::string name_;
        std::string stash_name_;
        std::size_t decode_jobs_ = 0;
    private:
        bool overwrite_ = false;
        bool discardtext_ = false;
        bool yenc_ = false;
    };

} // newsflash

