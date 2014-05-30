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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QJson/Serializer>
#  include <QJson/Parser>
#  include <QVariant>
#  include <QScopedPointer>
#  include <QIODevice>
#  include <QDebug>
#include <newsflash/warnpop.h>

#include <stdexcept>
#include <sstream>
#include "format.h"
#include "settings.h"

namespace app
{

settings::settings()
{}

settings::~settings()
{}


void settings::load(QIODevice& io, settings::format format)
{
    Q_ASSERT(io.isOpen());

    if (format == settings::format::json)
    {
        bool ok = false;
        QJson::Parser parser;
        auto ret = parser.parse(&io, &ok).toMap();
        if (!ok)
        {
            const auto& line = parser.errorLine();
            const auto& what = parser.errorString();
            throw std::runtime_error(
                str_a("json parse failed error '_1' at line _2", 
                    what, line));
        }

        QVariantMap db;

        for (auto it = ret.begin(); it != ret.end(); ++it)
        {
            const auto& context = it.key();
            const auto& values  = it.value().toMap();
            db[context] = values;
        }
        values_ = db;
    }
}

void settings::save(QIODevice& io, settings::format format) const
{
    Q_ASSERT(io.isOpen());

    if (format == settings::format::json)
    {
        bool ok = false;
        QJson::Serializer serializer;
        serializer.setIndentMode(QJson::IndentFull);
        serializer.serialize(values_, &io, &ok);
        if (!ok)
        {
            const auto& msg = serializer.errorMessage();
            throw std::runtime_error(
                str_a("json serialize failed '_1'", msg));
        }
    }
}

QVariant settings::get(const char* context, const char* name) const
{
    const QVariantMap& map = values_[context].toMap();
    if (map.isEmpty())
        return QVariant();

    const QVariant& value = map[name];
    return value;
}

QVariant settings::get(const char* context, const char* name, const QVariant& defval) const
{
    auto value = get(context, name);
    if (value.isNull())
        return defval;

    return value;
}

void settings::set(const char* context, const char* name, const QVariant& value)
{
    QVariantMap map = values_[context].toMap();
    map[name] = value;
    values_[context] = map;
}

void settings::clear()
{
    values_.clear();
}

bool settings::contains(const char* context, const char* name) const
{
    const auto& value = get(context, name);
    if (value.isNull())
        return false;

    return true;
}

} // app

