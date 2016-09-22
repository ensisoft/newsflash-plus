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
#include "newsflash/warnpush.h"
#include "newsflash/warnpop.h"
#include <cstddef>

class QString;
class QRegExp;

namespace gui
{
    // interface for Finding matching data in some datasource object.
    class Finder
    {
    public:
        virtual ~Finder() = default;

        // try to find str in the item at the given index.
        // returns true if the given str can be found in the item at the given index.
        // if caseSensitive is true then str is in upper case and upper case comparision 
        // should be evaluated. 
        // if there's no match return false.
        virtual bool isMatch(const QString& str, std::size_t index, bool caseSensitive) = 0;
        
        // match the item at the given index against the given regular expression.
        // returns true if the regular expression matches. otherwise false.
        virtual bool isMatch(const QRegExp& regex, std::size_t index) = 0;

        // return the number of items available.
        virtual std::size_t numItems() const = 0;

        // return the current index where the finding should begin.
        virtual std::size_t curItem() const = 0;

        // set the current index as "found".
        virtual void setFound(std::size_t index) = 0;

    private:
    };
    
} // gui