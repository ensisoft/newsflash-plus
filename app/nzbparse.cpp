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
#  include <QtXml/QXmlSimpleReader>
#  include <QtXml/QXmlInputSource>
#  include <QtXml/QXmlDefaultHandler>
#  include <QStack>
#include <newsflash/warnpop.h>

#include "nzbparse.h"

namespace {
class xmlhandler : public QXmlDefaultHandler
{
public:
    xmlhandler(QList<app::nzbcontent>& contents) : contents_(contents), nzb_error_(false), xml_error_(false)
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

            app::nzbcontent content {};
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
    QList<app::nzbcontent>& contents_;
    QStack<QString> stack_;
    QString error_;    
    bool nzb_error_;
    bool xml_error_;
    
};

} // 

namespace app
{

nzberror parse_nzb(QIODevice& io, QList<nzbcontent>& content)
{
    QList<nzbcontent> data;

    xmlhandler handler(data);
    QXmlInputSource source(&io);
    QXmlSimpleReader reader;
    
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    if (!reader.parse(&source))
    {
        if (!io.isOpen())
            return nzberror::io;
        else if (handler.nzb_error())
            return nzberror::nzb;

        return nzberror::xml;
    }
    content.append(data);

    return nzberror::none;    
}

} // app
