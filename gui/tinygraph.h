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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QWidget>
#  include <QtGui/QColor>
#  include <QtGui/QFont>
#  include <QTimer>
#  include <QString>
#  include <QList>
#include <newsflash/warnpop.h>

namespace gui
{
    class TinyGraph : public QWidget
    {
        Q_OBJECT

    public:
        TinyGraph(QWidget* parent);
       ~TinyGraph();

        struct colors {
            QColor grad1;
            QColor grad2;
            QColor outline;
            QColor fill;
        };

        // sample value is any arbitrary value
        // timestamp should be the timestamp of the sample in milliseconds
        void add_sample(double value, unsigned int timestamp);
        
        // set any text to be rendered within the graph
        void set_text(const QString& str);

        // set colors
        void set_colors(const colors& c);
        
        const colors& get_colors() const;

    private:
        void paintEvent(QPaintEvent* event);
        //void contextMenuEvent(QContextMenuEvent* event);

    private slots:
        void action_choose_1st_gradient();
        void action_choose_2nd_gradient();
        void action_choose_outline_color();
        void action_choose_fill_color();
        void action_choose_font();

    private:
        struct sample {
            double value;
            unsigned int timestamp;
        };

        QList<sample> timeline_;
        colors  colors_;
        QString text_;
        QFont   font_;
        double  maxval_;
        int     width_;
    };

} // gui



