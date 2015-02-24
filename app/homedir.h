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
#  include <QString>
#include <newsflash/warnpop.h>

namespace app
{
    // applications home directory in user home
    class homedir
    {
    public:
        // initialize once. 
        // folder is name for our application specific folder
        // in the user's real home. for example /home/roger/ on 
        // a linux system and "c:\Documents and Settings\roger\"
        // on a windows system, so we get 
        // "home/roger/folder" and "c:\documents and settings\roger\folder".
        static 
        void init(const QString& folder);

        // get absolute path to the applications home directory
        static 
        QString path();

        // get the path to a file in the home directory 
        // in system specific path notation.
        static 
        QString file(const QString& name);

    private:
        static QString pathstr;
    };

} // app
