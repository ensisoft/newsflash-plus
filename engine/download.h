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

#include "bitflag.h"
#include "datafile.h"
#include "task.h"

namespace newsflash
{
    class DecodeJob;

    class ContentTask : public Task
    {
    public:
        using WriteComplete = DataFile::WriteComplete;
        using OnWriteDone = DataFile::OnWriteDone;

        virtual void SetWriteCallback(const OnWriteDone& callback) = 0;
    };

    // extract encoded content from the buffers
    class Download : public ContentTask
    {
    public:
        Download(const std::vector<std::string>& groups,
            const std::vector<std::string>& articles,
            const std::string& path,
            const std::string& name);
        Download();

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
        virtual float GetProgress() const override;
        virtual bitflag<Error> GetErrors() const override;
        virtual void Pack(data::TaskState& data) const override;
        virtual void Load(const data::TaskState& data) override;

        // ContentTask implementation
        virtual void SetWriteCallback(const OnWriteDone& callback) override
        { callback_ = callback; }

        // allow read only accessors mostly for convenience in
        //  unit testing.
        std::string GetGroup(size_t i) const
        { return groups_[i]; }
        std::string GetArticle(size_t i) const
        { return articles_[i]; }
        std::size_t GetNumArticles() const
        { return articles_.size(); }
        std::size_t GetNumGroups() const
        { return groups_.size(); }

        std::size_t GetNumFiles() const
        { return files_.size(); }
        const DataFile* GetFile(size_t i) const
        { return files_[i].get(); }

    private:
        std::shared_ptr<DataFile> create_file(const std::string& name, std::size_t assumed_size);

    private:
        using stash = std::vector<char>;

        std::vector<std::string> groups_;
        std::vector<std::string> articles_;
        std::vector<std::shared_ptr<DataFile>> files_;
        std::vector<std::unique_ptr<stash>> stash_;
        std::string path_;
        std::string name_;
        std::string stash_name_;
        std::size_t num_decode_jobs_   = 0;
        std::size_t num_actions_total_ = 0;
        std::size_t num_actions_ready_ = 0;
    private:
        bitflag<Error> errors_;
    private:
        bool overwrite_ = false;
        bool discardtext_ = false;
    private:
        OnWriteDone callback_;
    };

} // newsflash

