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

#include <set>
#include <memory>
#include <cstdint>
#include "taskcmd.h"
#include "taskio.h"

namespace newsflash
{
    class update
    {
    public:
        class cmd : public taskcmd
        {
        public:
            virtual std::size_t enqueue(cmdqueue& cmds, std::size_t task, std::size_t limit) override
            {
                return 0;
            }

            virtual std::size_t complete(std::unique_ptr<command> cmd) override
            {
                return 0;
            }
        private:
        };

        class io : public taskio
        {
        public:
            enum class flags {
                merge_old       = 0x1,
                mark_old_new    = 0x2,
                index_by_author = 0x4,
                index_by_date   = 0x8,
                ids64           = 0x10,
                crc16           = 0x20
            };

            io(std::string key);
           ~io();

            virtual void prepare() override;
            virtual void receive(const buffer& buff) override;
            virtual void cancel() override;
            virtual void flush() override;
            virtual void finalize() override;

        private:
            const std::string key_;

        private:

        };

        static 
        std::pair<std::unique_ptr<taskcmd>,
                  std::unique_ptr<taskio>> create(std::string server, std::string group)
        {
            const std::string& key = server + "/" + group;
            if (updates_.find(key) != updates_.end())
                throw std::runtime_error("update already in progress for " + key);

            return {
                std::unique_ptr<taskcmd> { new cmd },
                std::unique_ptr<taskio>  { new io{key}}
            };

        }
    private:
        static 
        void erase_key(const std::string& key)
        {
            updates_.erase(key);
        }

        friend class io;

        static std::set<std::string> updates_; 
    };
} // newsflash
