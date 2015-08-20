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

#include <newsflash/config.h>
#include <functional>
#include <memory>
#include <string>
#include <cstdint>
#include "task.h"

namespace newsflash
{
    class update : public task
    {
    public:
        // callback that is fired when new data has been written
        // to some of the data files belonging to the group.
        std::function<void(const std::string& group,
            const std::string& file)> on_write;

        // callback that is fired when information about the
        // group is discovered.
        std::function<void(const std::string& group,
            std::uint64_t local, std::uint64_t remote)> on_info;

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

        virtual std::size_t max_num_actions() const override;

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
        std::uint64_t remote_last_;
        std::uint64_t remote_first_;
        std::uint64_t local_last_;
        std::uint64_t local_first_;
        std::uint64_t xover_last_;
        std::uint64_t xover_first_;
    };

} // newsflash
