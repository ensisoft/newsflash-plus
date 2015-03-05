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

#define LOGTAG "search"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QIODevice>
#  include <QByteArray>
#  include <QUrl>
#include <newsflash/warnpop.h>
#include "searchengine.h"
#include "indexer.h"
#include "debug.h"
#include "eventlog.h"
#include "search.h"

namespace app
{

SearchEngine::SearchEngine()
{
    net_ = g_net->getSubmissionContext();

    DEBUG("SearchEngine created");
}

SearchEngine::~SearchEngine()
{
    DEBUG("SearchEngine deleted");
}

Search SearchEngine::beginSearch(const Basic& query)
{
    
}

void SearchEngine::addEngine(std::unique_ptr<Indexer> engine)
{
    engines_.push_back(std::move(engine));
}

} // app
