// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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
#include <string>
#include <vector>

#include "cmdlist.h"
#include "buffer.h"

namespace newsflash
{
    // retrieve a list of nntp articles known as the messages.
    // the messages are expected to be found in one of the  given newsgroups.
    class bodylist : public cmdlist
    {
    public:
        bodylist(std::size_t task, std::size_t account, 
            std::vector<std::string> groups,
            std::vector<std::string> messages);
       ~bodylist();

        virtual bool submit_configure_command(std::size_t i, session& ses) override;

        virtual bool receive_configure_buffer(std::size_t i, buffer&& buff) override;

        virtual void submit_data_commands(session& ses) override;

        virtual void receive_data_buffer(buffer&& buff) override;

        virtual std::size_t task() const override
        { return task_; }

        virtual std::size_t account() const override
        { return account_; }

        std::vector<buffer> get_buffers()
        {
            std::vector<buffer> ret;
            for (auto& buff : buffers_)
                ret.push_back(std::move(buff));

            return ret;
        }

        void set_account(std::size_t acc)
        { account_ = acc; }


        const 
        std::vector<std::string>& get_messages() const 
        { return messages_; }

        const
        std::vector<std::string>& get_groups() const
        { return groups_; }

        bool is_complete() const 
        { return message_ == messages_.size(); }

        bool configure_fail() const
        { return configure_fail_bit_; }

        void clear_fail_bits() 
        { configure_fail_bit_ = false; }

        std::size_t num_messages() const 
        { return messages_.size(); }

    private:
        const std::size_t task_;        
        const std::vector<std::string> groups_;
        const std::vector<std::string> messages_;
    private:
        std::vector<buffer> buffers_;
        std::size_t account_;
        std::size_t message_;
    private:
        bool configure_fail_bit_;
    }; 

} // newsflash