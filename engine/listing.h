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

#include <string>
#include <cstdint>
#include <vector>
#include "task.h"

namespace newsflash
{
    // produce a listing of available newsgroups
    class listing : public task
    {
    public:
        struct group {
            std::string name;            
            std::uint64_t last;
            std::uint64_t first;
            std::uint64_t size;
        };

        listing() : run_once_(true)
        {}

        virtual std::shared_ptr<cmdlist> create_commands() override;

        virtual void complete(cmdlist& cmd, 
            std::vector<std::unique_ptr<action>>& actions) override;

        virtual bool has_commands() const override
        {
            return run_once_;
        }

        const std::vector<group>& group_list() const 
        {
            return groups_;
        }
    private:
        std::vector<group> groups_;
    private:
        bool run_once_;
    };

} // newsflash
