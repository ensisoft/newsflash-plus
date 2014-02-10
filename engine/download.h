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
#include <string>
#include "taskcmd.h"
#include "taskio.h"

namespace newsflash
{
    class download
    {
    private:
        class cmd : public taskcmd
        {
        public:
            virtual std::size_t enqueue(cmdqueue& cmds, std::size_t task, std::size_t limit) override;
            virtual std::size_t complete(std::unique_ptr<command> cmd) override;
        private:
        };

        class io : public taskio
        {
        public:
            io(std::string folder, std::string name);

            virtual void prepare() override;
            virtual void receive(const buffer& buff) override;
            virtual void cancel() override;
            virtual void flush() override;
            virtual void finalize() override;
        private:
            const std::string folder_;
            const std::string name_;
        private:
            struct file;

        private:
            std::vector<file*> files_;

        };

    public:
        static
        std::unique_ptr<taskio> create_io(std::string folder, std::string name)
        {
            return std::unique_ptr<taskio> { new io{folder, name} };
        }
        static
        std::unique_ptr<taskcmd> create_cmd()
        {
            return std::unique_ptr<taskcmd> { new cmd };
        }

    };

} //  newsflash