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
        void addSample(unsigned value);
        
        // set any text to be rendered within the graph
        void setText(const QString& str);

        // set colors
        void setColors(const colors& c);
        
        const colors& getColors() const;

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
            unsigned value;
            unsigned timestamp;
        };

        QList<sample> timeline_;
        colors  colors_;
        QString text_;
        QFont   font_;
        unsigned maxval_;
    };

} // gui



