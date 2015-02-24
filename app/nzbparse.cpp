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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtXml/QXmlSimpleReader>
#  include <QtXml/QXmlInputSource>
#  include <QtXml/QXmlDefaultHandler>
#  include <QStack>
#include <newsflash/warnpop.h>
#include <algorithm>
#include "nzbparse.h"

namespace {
class MyHandler : public QXmlDefaultHandler
{
public:
    MyHandler(std::vector<app::NZBContent>& contents) : contents_(contents), nzb_error_(false), xml_error_(false)
    {}

    virtual bool characters(const QString& ch) override
    {
        if (!has_content())
            return true;

        auto& content = contents_.back();

        // process the data of an xml element
        if (curr_element_is("group"))
        {
            // XML parser gives us empty (\r\n) lines. let's skip these
            const QString& group = ch.trimmed();
            if (group.isEmpty())
                return true;

            content.groups.push_back(group.toStdString());
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

            content.segments.push_back(segment.toStdString());
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

            app::NZBContent content {};
            content.subject = attrs.value("subject");
            if (content.subject.isEmpty())
                return false;            
            content.date    = attrs.value("date");
            content.poster  = attrs.value("poster");
            content.type    = app::FileType::None;//app::find_filetype(content.subject);
            contents_.push_back(content);
        }
        else if (element == "segment")
        {
            if (!curr_element_is("segments"))
                return false;

            if (!has_content())
                return false;

            auto& content = contents_.back();
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
        return !contents_.empty();
    }

private:
    std::vector<app::NZBContent>& contents_;
    QStack<QString> stack_;
    QString error_;    
    bool nzb_error_;
    bool xml_error_;
    
};

} // 

namespace app
{

NZBError parseNZB(QIODevice& io, std::vector<NZBContent>& content)
{
    std::vector<NZBContent> data;

    MyHandler handler(data);
    QXmlInputSource source(&io);
    QXmlSimpleReader reader;
    
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    if (!reader.parse(&source))
    {
        if (!io.isOpen())
            return NZBError::Io;
        else if (handler.nzb_error())
            return NZBError::NZB;

        return NZBError::XML;
    }
    //content.append(data);
    std::copy(std::begin(data), std::end(data), std::back_inserter(content));
    return NZBError::None;    
}

} // app
