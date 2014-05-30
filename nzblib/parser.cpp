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

#include <newsflash/warnpush.h>
#  include <QtXml/QXmlSimpleReader>
#  include <QtXml/QXmlInputSource>
#  include <QtXml/QXmlDefaultHandler>
#  include <QStack>
#include <newsflash/warnpop.h>
#include "parser.h"

namespace {
    
class xmlhandler : public QXmlDefaultHandler
{
public:
    xmlhandler(QList<nzb::content>& contents) : contents_(contents), nzb_error_(false), xml_error_(false)
    {}

    virtual bool characters(const QString& ch) override
    {
        Q_ASSERT(has_content());

        auto& content = contents_.last();

        // process the data of an xml element
        if (curr_element_is("group"))
        {
            // XML parser gives us empty (\r\n) lines. let's skip these
            const QString& group = ch.trimmed();
            if (group.isEmpty())
                return true;

            content.groups.append(group);
        }
        else if (curr_element_is("segment"))
        {
            // xml parser gives us empty (\r\n) lines. let's skip these
            QString segment = ch.trimmed();
            if (segment.isEmpty())
                return true;

            // nzb file format specification says that the message-ids are without 
            // the enclosing angle brackets
            // http://docs.newzbin.com/index.php/Newzbin:NZB_Specs
            if (segment[0] != '<')
                segment.prepend('<');
            if (segment[segment.size()-1] != '>')
                segment.append('>');

            content.segments.append(segment);
        }
        return true;
    }
    virtual bool startElement(const QString& namespace_uri, const QString& localname, const QString& name, const QXmlAttributes& attrs) override
    {
        Q_UNUSED(namespace_uri);
        Q_UNUSED(localname);
        
        nzb_error_ = true;

        const QString& element = name.toLower();
        if (element == "file")
        {
            if (!curr_element_is("nzb"))
                return false;

            nzb::content content {};
            content.subject = attrs.value("subject");
            content.date    = attrs.value("date");
            content.poster  = attrs.value("poster");
            if (content.subject.isEmpty())
                return false;
            
            contents_.append(content);
        }
        else if (element == "segment")
        {
            if (!curr_element_is("segments"))
                return false;

            if (!has_content())
                return false;

            auto& content = contents_.last();
            content.bytes += attrs.value("bytes").toInt();
        }
        else if (element == "group")
        {
            if (!curr_element_is("groups"))
                return false;

            if (!has_content())
                return false;
        }
        stack_.push(element);
        nzb_error_ = false;
        return true;
    }
    virtual bool endElement(const QString& namespace_uri, const QString& localname, const QString& name) override
    {
        Q_UNUSED(namespace_uri);
        Q_UNUSED(localname);

        if (stack_.isEmpty())
            return false;

        const auto& top = stack_.top();
        if (top != name)
            return false;

        stack_.pop();
        return true;
    }

    virtual bool fatalError(const QXmlParseException& exception) override
    {
        // could store the error information here for further analysis
        error_ = exception.message();
        xml_error_ = true;
        return false;
    }
    
    bool nzb_error() const
    {
        return nzb_error_;
    }

    bool xml_error() const
    {
        return xml_error_;
    }

    QString error() const
    {
        return error_;
    }
private:
    bool curr_element_is(const QString& name) const
    {
        if (stack_.isEmpty())
            return false;

        const auto& top = stack_.top();
        if (top != name)
            return false;
        return true;
    }
    bool has_content() const
    {
        return !contents_.isEmpty();
    }

private:
    QList<nzb::content>& contents_;
    QStack<QString> stack_;
    QString error_;    
    bool nzb_error_;
    bool xml_error_;
    
};

} // namespace

namespace nzb
{

error parse(QIODevice& input, QList<content>& contents, QString* errstr)
{
    QList<content> data;

    xmlhandler handler(data);
    QXmlInputSource source(&input);
    QXmlSimpleReader reader;
    
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    if (!reader.parse(&source))
    {
        if (!input.isOpen())
            return error::io;
        else if (handler.nzb_error())
            return error::nzb;

        if (errstr)
            *errstr = handler.error();

        return error::xml;
    }
    contents.append(data);

    return error::none;
}

// remember that virtual functions are not virtual inside ctor and dtor!

parser::parser() : shutdown_(false)
{}

parser::~parser()
{
    Q_ASSERT(shutdown_);
}

void parser::shutdown()
{
    if (isRunning())
    {
        QMutexLocker lock(&mutex_);        
        shutdown_ = true;
        cond_.wakeOne();
    }
    wait();    
}

void parser::parse(const QString& file)
{
    QMutexLocker lock(&mutex_);

    files_.append(file);

    if (!isRunning())
        start();

    cond_.wakeOne();
}

void parser::run()
{
    for (;;)
    {
        QString file;
        QMutexLocker lock(&mutex_);
        if (!shutdown_ && files_.isEmpty())
        {
            cond_.wait(&mutex_);
            continue; // takes care of sporadic wakeup
        }
        else if (shutdown_)
            break;

        file = files_.first();
        files_.pop_front();
        lock.unlock();

        parser::data data;
        data.error = error::none;
        data.file  = file;

        QFile input(file);
        if (!input.open(QIODevice::ReadOnly))
             data.error = error::io;
        else data.error = nzb::parse(input, data.contents);

        lock.relock();
        data_.push_back(data);
        emit dataready();
    }
}

bool parser::get(parser::data& data)
{
    QMutexLocker lock(&mutex_);
    if (data_.isEmpty())
        return false;

    data = std::move(data_.first());
    data_.pop_front();
    return true;
}

} // nzb
