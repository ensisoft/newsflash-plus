// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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

#define MODEL_IMPL
#define LOGTAG "womble"

#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/format.h>

#include <newsflash/warnpush.h>
#  include <QtXml/QDomDocument>
#  include <QIODevice>
#  include <QDateTime>
#  include <QUrl>
#include <newsflash/warnpop.h>

#include <cstring>
#include "womble.h"
//#include <rsslib/rss.h>

namespace womble
{

plugin::plugin(sdk::hostapp& host) : host_(host)
{
    DEBUG("Womble created");
}

plugin::~plugin()
{
    DEBUG("Womble created");
}

void plugin::load_content()
{
    
}

QAbstractItemModel* plugin::view() 
{
    return nullptr;
}

QString plugin::name() const
{
    return "womble";
}

} // womble


MODEL_API sdk::model* create_model(sdk::hostapp* host)
{
    try 
    {
        return new womble::plugin(*host);
    }
    catch (const std::exception& e)
    {
        ERROR(sdk::str("Womble exception _1", e.what()));
    }
    return nullptr;
}

MODEL_API int model_api_version()
{
    return sdk::model::version;
}

MODEL_API void model_lib_version(int* major, int* minor)
{
    *major = 1;
    *minor = 0;
}
