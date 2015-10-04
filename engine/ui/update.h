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
#include <string>
#include <cstdint>

namespace newsflash
{
    namespace ui {

    // details for newsgroup update task
    struct update 
    {
        std::size_t account;

        // path in the filesystem where to place the data files.
        std::string path;

        // name of the newsgroup to update.
        std::string name;

        // human readable descripton for the task list.
        std::string desc;

        // this will be the number of articles stored locally
        // when an update is completed.
        std::uint64_t num_local_articles;

        // this will be the total number of articles availble
        // on the server at the completion of the update.
        std::uint64_t num_remote_articles;
    };

    } // ui

} // newsflash