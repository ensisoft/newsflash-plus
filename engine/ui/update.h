// Copyright (c) 2010-2017 Sami Väisänen, Ensisoft
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
#include <cstdint>
#include <vector>

namespace newsflash
{
    // todo: maybe refactor this to some other place from the catalog.h
    struct Snapshot;

    namespace ui {

    struct Update
    {
        virtual ~Update() = default;

        std::size_t account = 0;
    };

    struct HeaderUpdate : public Update
    {
        // name of the newsgroup in question.
        std::string group_name;

        // the number of articles on locally indexed.
        std::uint64_t num_local_articles  = 0;

        // the number of remote articles available on the server.
        std::uint64_t num_remote_articles = 0;

        // list of catalog files that were updated
        std::vector<std::string> catalogs;

        // list of catalog snapshots each snapshot
        // corresponding to a file in the catalogs vector.
        std::vector<const Snapshot*> snapshots;
    };

    struct GroupListUpdate : public Update
    {
        struct NewsGroup {
            // name of the newsgroup, for example alt.binaries.movies.divx
            std::string name;

            // first article id in the group (low water mark)
            std::uint64_t first = 0;

            // last article id in the group (high water mark)
            std::uint64_t last = 0;

            // estimated number of articles
            std::uint64_t size = 0;
        };
        std::vector<NewsGroup> groups;
    };

    } // namespace

} // namespace
