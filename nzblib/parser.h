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

#pragma once

#include <QWaitCondition>
#include <QMutex>
#include <QThread>
#include <QString>
#include <QList>
#include <QObject>
#include <QtGlobal>

class QIODevice;

// NZB is an XML based file format for retrieving posts from Usenet 
// (basically nzb files are the .torrent files of usenet)

namespace nzb
{
    // content data parsed from a nzb file
    struct content
    {
        // Subject line
        QString subject;

        // Post date (unix time)
        QString date;

        // Poster (also known as Author)
        QString poster;

        // The  list of newsgroups where the file has been posted
        QList<QString> groups;

        // The list of segments (message-ids) that comprise this file
        QList<QString> segments;

        // the presumed size of the content in bytes
        quint64 bytes;
    };

    // nzb parsing error codes
    enum class error {
        none, xml, nzb, io, other
    };
    
    // parse NZB input from input* and read parse details
    // into a list of file objects.
    // returns one of the enumerated error codes on return.
    error parse(QIODevice& input, QList<content>& contents, QString* errstr = nullptr);

    // parse a nzb file on the background
    class parser : public QThread
    {
        Q_OBJECT
    public:
        struct data {
            nzb::error error;
            QString file;
            QList<content> contents;
        };

        parser();
       ~parser();

        void shutdown();

        // parse the specified file. after the parsing is done a signal is emitted
        void parse(const QString& file);
        
        // get parsed data from the queue. returns true
        // if succesful (there was data available) or false if no data was available.
        bool get(parser::data& data);

    signals:
        void dataready();

    private:
        virtual void run() override;

    private:
        QWaitCondition cond_;        
        QMutex mutex_;
        QList<data> data_; // parsed and ready data
        QList<QString> files_; // files to be parsed
        bool shutdown_;
    };

} // nzb



