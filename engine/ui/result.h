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

#include <string>
#include <cstddef>
#include <vector>

namespace newsflash
{
    namespace ui {

    // a new file produced by downloading and decoding content
    struct FileResult
    {
        // complete path to the file location.
        std::string path;

        // the name of the file.
        std::string name;

        // the size of the file that was produced.
        std::uint64_t size = 0;

        // this is set to true if contents are suspected to be damaged
        bool damaged = false;

        // this is set to true if the file is binary data
        bool binary = false;
    };

    struct FileBatchResult
    {
        // complete path to the batch location
        std::string path;

        // the human readable description of the batch.
        std::string desc;

    };

    struct GroupListResult
    {
        std::size_t account = 0;

        std::string desc;

        struct Newsgroup {
            // name of the newsgroup for example alt.binaries.movies.divx
            std::string name;

            // first article id in the group (low water mark)
            std::uint64_t first = 0;

            // last article id in the group (high water mark)
            std::uint64_t last = 0;

            // estimated number of articles.
            std::uint64_t size = 0;
        };
        std::vector<Newsgroup> groups;
    };

    struct HeaderResult
    {
        // the account to be used to download the headers from.
        std::size_t account = 0;

        // path in the filesystem where to place the data files.
        std::string path;

        // name of the newsgroup to update.
        std::string group;

        // human readable descripton for the task list.
        std::string desc;

        // this will be the number of articles stored locally
        // when an update is completed.
        std::uint64_t num_local_articles = 0;

        // this will be the total number of articles availble
        // on the server at the completion of the update.
        std::uint64_t num_remote_articles = 0;
    };

} // ui
} // engine
