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

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

namespace newsflash
{
    namespace ui {

    // a download job details for downloading single item of content (a file)
    struct FileDownload
    {
        // list of message id's or message numbers.
        // note that message numbers are specific to a server
        // while message-ids are portable across servers.
        std::vector<std::string> articles;

        // the list of groups into which look for the messge ids.
        std::vector<std::string> groups;

        // the esimated size of the file to be downloaded in bytes.
        // 0 if not known.
        std::uint64_t size = 0;

        // the local filesystem path where the downloadded/decoded content
        // is to the be placed.
        std::string path;

        // the human readable description that will appear in task::description.
        // usually the expected name of the file.
        std::string name;
    };

    // message-id's are unique across all Usenet servers and have are variable
    // length strings enclosed in angle brackets '<' and '>'.
    // if the articles in the download are message-ids we the download is
    // "fillable", i.e. another server can queried for the messages in case
    // they are missing from the primary server.
    inline bool is_fillable(const FileDownload& d)
    {
        for (const auto& a : d.articles)
        {
            if (a.front() != '<' || a.back() != '>')
                return false;
        }
        return true;
    }

    // A set of FileDownloads grouped together into a "batch"
    struct FileBatchDownload
    {
        // the account to be used to download the files in the batch.
        std::size_t account = 0;

        // the path on the local file system where the files are to be placed
        std::string path;

        // the human readable description for the whole batch
        std::string desc;

        // the descriptions for the files to be downloaded.
        std::vector<FileDownload> files;
    };

    // download news group listing
    struct GroupListDownload
    {
        // the account to be used to download the listing from
        std::size_t account = 0;

        // human readable description for the job to be done.
        std::string desc;
    };

    // details for downloading the headers for some particular news group
    struct HeaderDownload
    {
        // the account to be used to download the headers from.
        std::size_t account = 0;

        // path in the filesystem where to place the data files.
        std::string path;

        // name of the newsgroup to update.
        std::string group;

        // human readable descripton for the task list.
        std::string desc;
    };

} // ui
} // engine
