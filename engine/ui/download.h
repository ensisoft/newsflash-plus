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

    struct Download {
        // the account used to perform the download
        std::size_t account = 0;

        // the estimated size of the file to be downloaded in bytes.
        // 0 if not known.
        std::uint64_t size = 0;

        // the local filesystem path where the downloadded/decoded content
        // is to the be placed.
        std::string path;

        // the human readable description that will appear in task::description.
        // usually the expected name of the file or some other descriptive text.
        std::string desc;
    };

    // a download job details for downloading single item of content (a file)
    struct FileDownload : public Download
    {
        // list of message id's or message numbers.
        // note that message numbers are specific to a server
        // while message-ids are portable across servers.
        std::vector<std::string> articles;

        // the list of groups into which look for the messge ids.
        std::vector<std::string> groups;
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
    struct FileBatchDownload : public Download
    {
        // the descriptions for the files to be downloaded.
        std::vector<FileDownload> files;
    };

    // download news group listing
    struct GroupListDownload : public Download
    {
        // no special parameters here.
    };

    // details for downloading the headers for some particular news group
    struct HeaderDownload : public Download
    {
        // name of the newsgroup to update.
        std::string group;
    };

} // ui
} // engine
