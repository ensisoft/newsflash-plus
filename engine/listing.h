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

#include <memory>
#include <vector>
#include <cstdint>
#include "taskcmd.h"
#include "taskio.h"

namespace newsflash
{
    class listing
    {
    private:
        class cmd : public taskcmd
        {
        public:
            virtual std::size_t enqueue(cmdqueue& cmds, std::size_t task, std::size_t limit) override
            {
                // request for the newsgroup listing.
                cmds.push_back(new cmd_list(0, task));
                return 1;
            }
            virtual std::size_t complete(std::unique_ptr<command> cmd) override
            {
                // the command is not supposed to fail so we're done
                // for the commands here.
                return 0;
            }
        private:
        };

        class io : public taskio
        {
        public:
            io(std::string filename);

            virtual void prepare() override;
            virtual void receive(const buffer& buff) override;
            virtual void cancel() override;
            virtual void flush() override;
            virtual void finalize() override;
        private:
            const std::string filename_;

        private:
            struct group_info {
                std::string name; // group name
                std::uint64_t size;
            };

            std::vector<group_info> groups_;
        };

    public:
        static
        std::unique_ptr<taskio> create_io(std::string filename)
        {
            return std::unique_ptr<taskio> { new io{filename }};
        }

        static 
        std::unique_ptr<taskcmd> create_cmd()
        {
            return std::unique_ptr<taskcmd> { new cmd };
        }
    private:
    };
} // newsflash
