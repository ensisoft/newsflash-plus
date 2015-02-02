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
#include "settings.h"
#include "format.h"

namespace app
{

Settings::Settings()
{}

Settings::~Settings()
{}


void Settings::load(QIODevice& io, Settings::format format)
{
    Q_ASSERT(io.isOpen());

    if (format == Settings::format::json)
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

void Settings::save(QIODevice& io, Settings::format format) const
{
    Q_ASSERT(io.isOpen());

    if (format == Settings::format::json)
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

QFile::FileError Settings::load(const QString& file, Settings::format format)
{
    QFile io(file);
    if (!io.open(QIODevice::ReadOnly))
        return io.error();

    load(io, format);
    return QFile::NoError;
}

QFile::FileError Settings::save(const QString& file, Settings::format format)
{
    QFile io(file);
    if (!io.open(QIODevice::WriteOnly))
        return io.error();

    save(io, format);
    return QFile::NoError;
}

void Settings::clear()
{
    values_.clear();
}

bool Settings::contains(const char* context, const char* name) const
{
    const auto& value = get(context, name);
    if (value.isNull())
        return false;

    return true;
}

QVariant Settings::get(const QString& key, const QString& attr, const QVariant& defval) const
{
    const QVariantMap& map = values_[key].toMap();
    if (map.isEmpty())
        return defval;

    const QVariant& value = map[attr];
    return value;
}

void Settings::set(const QString& key, const QString& attr, const QVariant& value)
{
    QVariantMap map = values_[key].toMap();
    map[attr] = value;
    values_[key] = map;
}

void Settings::del(const QString& key)
{
    values_.remove(key);
}

Settings* g_settings;

} // app

