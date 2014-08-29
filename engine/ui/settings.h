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

#include <newsflash/config.h>

#include <cstddef>

namespace newsflash
{
    namespace ui {
    // engine settings
    struct settings
    {
        // if set to true engine will overwrite files
        // that already exist in the filesystem.
        // if set to false, new files will be renamed
        // to avoid collisions.
        bool overwrite_existing_files;

        // set to true to discard any textual data 
        // that is downloaded as a part of downloading
        // some binary content.
        // set to false to have the text data stored to the disk.
        bool discard_text_content;

        // set to true to automatically prune the task list
        // and kill completed items.
        bool auto_remove_completed;

        bool auto_connect;

        // set to true to choose the secure server over the
        // general server with accounts that provide 
        // secure server. if no secure server is available
        // then general server is used.
        bool prefer_secure;

        // set to true to enable network throttlin to converse
        // bandwidth for other processes. if set to true
        // throttle specifies the actual value.
        bool enable_throttle;

        // set to true to enable the use of any fillservers.
        // fillservers are used when the primary download
        // server has missing data.
        bool enable_fill_account;

        std::size_t fill_account;

        // throttle speed to this limit. specified in bytes per second.
        int throttle;

    };

    } // ui
    
} // engine
