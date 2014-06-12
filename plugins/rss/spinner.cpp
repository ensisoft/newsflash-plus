// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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
#  include <QtGui/QPainter>
#  include <QtGui/QColor>
#include <newsflash/warnpop.h>

#include "spinner.h"

namespace gui
{

spinner::spinner(QWidget* parent) : QWidget(parent), pos_(0)
{
    connect(&timer_, SIGNAL(timeout()), this, SLOT(update()));
}
spinner::~spinner()
{
}
void spinner::start()
{
    timer_.start(50);
}

void spinner::stop()
{
    timer_.stop();
}

bool spinner::is_active() const
{
    return timer_.isActive();
}

void spinner::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    enum { TICKS = 27 };

    unsigned pos = pos_++ % TICKS;

    int width  = this->width();
    int height = this->height();

    p.setRenderHint(QPainter::Antialiasing);
    
    const QColor tick(QColor(0xff, 0x8c, 00));
    
    p.translate(width / 2, height / 2);
    p.rotate((360/TICKS * pos) % 360);

    int y = -height / 2;

    for (int i=0; i<TICKS;++i)
    {
        QColor c(tick);
        if (is_active())
            c = c.lighter(100 + 2 * i);

        QPen pen(c);
        pen.setWidth(1);
        p.setPen(pen);
        p.rotate(360/TICKS);
        p.drawLine(0, y+1, 0, y + 8);
    }    
}

} // gui
