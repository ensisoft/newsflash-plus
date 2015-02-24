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

#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#  include <QString>
#include <newsflash/warnpop.h>
#include <string>
#include <vector>
#include "filetype.h"

class QIODevice;

namespace app
{
    // content data parsed from a NZB file
    struct NZBContent {
        // filetype is here for convenience, but its not parsed
        FileType type;

        // filetype icon is here for convenience but its not parsed
        QIcon icon;

        //subject line
        QString subject;

        // post date (unix time)
        QString date; 

        // Poster aka author
        QString poster; 

        // the list of newsgroups where the file has been posted
        std::vector<std::string> groups;

        // the list of message-ids that comprise this content
        std::vector<std::string> segments;

        // the presumed size of the content in bytes
        quint64 bytes;
    };

    enum class NZBError {
        None, XML, NZB, Io, Other
    };

    // parse nzb content from from input and store in the list of contents.
    // returns nzberror code indicating the result of the parsing.
    NZBError parseNZB(QIODevice& io, std::vector<NZBContent>& content);

} // app
