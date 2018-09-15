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
#include <cstdint>
#include "task.h"

namespace newsflash
{
    struct Snapshot;

    class HeaderTask : public Task
    {
    public:
        struct Progress {
            std::string group;
            std::string path;
            std::uint64_t num_local_articles  = 0;
            std::uint64_t num_remote_articles = 0;
            std::vector<std::unique_ptr<Snapshot>> snapshots;
            std::vector<std::string> catalogs;
        };
        using OnProgress = std::function<void (const Progress&)>;

        virtual void SetProgressCallback(const OnProgress& callback) = 0;
    private:
    };

    class Update : public HeaderTask
    {
    public:
        Update(const std::string& path, const std::string& group);
       ~Update();

        // Task implementation
        virtual std::shared_ptr<CmdList> CreateCommands() override;
        virtual void Cancel() override;
        virtual void Commit() override;
        virtual void Complete(CmdList& cmd,
            std::vector<std::unique_ptr<action>>& next) override;
        virtual void Complete(action& a,
            std::vector<std::unique_ptr<action>>& next) override;
        virtual bool HasCommands() const override;
        virtual bool HasProgress() const override;
        virtual float GetProgress() const override;
        virtual void Lock() override;
        virtual void Unlock() override;

        // HeaderTask implementation
        virtual void SetProgressCallback(const HeaderTask::OnProgress& callback) override;

        std::string group() const;

        std::string path() const;

        std::uint64_t num_local_articles() const;

        std::uint64_t num_remote_articles() const;

    private:
        class parse;
        class store;
        struct state;

    private:
        std::shared_ptr<state> state_;
        std::uint64_t remote_last_ = 0;
        std::uint64_t remote_first_ = 0;
        std::uint64_t local_last_ = 0;
        std::uint64_t local_first_ = 0;
        std::uint64_t xover_last_ = 0;
        std::uint64_t xover_first_ = 0;
    private:
        bool commit_done_ = false;
    private:
    };

} // newsflash
