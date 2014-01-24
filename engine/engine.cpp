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

#include <thread>
#include <vector>

#include "engine.h"
#include "listener.h"
#include "connection.h"
#include "command.h"
#include "response.h"
#include "logging.h"

namespace newsflash
{

class engine::impl
{
public:
    impl(listener& list, const std::string& logs) : logfolder_(logs), listener_(list)
    {

    }


private:
    const std::string& logfolder_;
    std::thread thread_;
    listener& listener_;    
    cmdqueue commands_;
    resqueue responses_;
};


engine::engine(listener& list, const std::string& logfolder)
{
    pimpl_.reset(new engine::impl(list, logfolder));
}


} // newsflash

