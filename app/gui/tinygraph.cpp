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
#  include <QtGui/QColor>
#  include <QtGui/QPainter>
#  include <QtGui/QGradient>
#  include <QtGui/QBrush>
#  include <QtGui/QColorDialog>
#  include <QtGui/QFontDialog>
#  include <QtGui/QMenu>
#  include <QtGui/QContextMenuEvent>
#  include <QtGui/QAction>
#  include <QVector>
#include <newsflash/warnpop.h>
#include <chrono>
#include "tinygraph.h"

namespace {
    enum {
        PIXELS_PER_SECOND = 5
    };

} // namespace

namespace gui
{

TinyGraph::TinyGraph(QWidget* parent)  : QWidget(parent), maxval_(0)
{
    colors_.grad1   = QColor(0, 0, 0);
    colors_.grad2   = QColor(0, 0, 0);
    colors_.fill    = QColor(255, 255, 255);
    colors_.outline = QColor(255, 255, 255);
}

TinyGraph::~TinyGraph()
{}


void TinyGraph::addSample(unsigned value)
{
    using clock = std::chrono::steady_clock;
    const auto now  = clock::now();
    const auto gone = now.time_since_epoch();
    const auto ms   = std::chrono::duration_cast<std::chrono::milliseconds>(gone);
    const auto timestamp = (unsigned)ms.count();

    sample s = { value, timestamp };
    timeline_.append(s);

    maxval_ = std::max(value, maxval_);

    // how many milliseconds worth of history are we showing?
    const auto width  = this->width();
    const auto millis = ((double)width / PIXELS_PER_SECOND) * 1000 + 5000;

    // crop samples that are tool old
    while (!timeline_.isEmpty())
    {
        const sample& s = timeline_[0];
        if (s.timestamp >= (timestamp - millis))
            break;
        
        timeline_.removeFirst();
    }

    update();
}

void TinyGraph::setText(const QString& str)
{
    text_ = str;
    update();
}

void TinyGraph::setColors(const colors& c)
{
    colors_ = c;
    update();
}

const TinyGraph::colors& TinyGraph::getColors() const
{
    return colors_;
}

void TinyGraph::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    
    int width  = this->width();
    int height = this->height();

    QLinearGradient grad(QPointF(0, 0), QPoint(0, height));
    grad.setColorAt(0, colors_.grad1);
    grad.setColorAt(1, colors_.grad2);

    // fill background with the gradient
    QBrush background(grad);
    p.fillRect(rect(), background);

    QPen black(QColor(0, 0, 0, 100));
    p.setPen(black);
    
    // draw the time grid
    int stride_in_px = PIXELS_PER_SECOND * 10; // 10s
    for (int x = width; x > 0; x -= stride_in_px)
        p.drawLine(x, 0, x, height);

    // draw the y-axis scale grid at 50% (half way)
    //p.drawLine(20, height/2-1, width, height/2-1);
    // draw y-axis legend
    /*
    QFont f;
    f.setPointSize(6);
    p.setFont(f);
    p.drawText(0, 6, "100%");
    p.drawText(0, height/2 + 3, "50%");
    p.drawText(0, height, "0%");
    */

    if (!text_.isEmpty())
    {
        p.setFont(font_);
        p.drawText(this->rect(), Qt::AlignCenter, text_);
    }

    if (timeline_.isEmpty() || maxval_ == 0.0)
        return;

    QPen outline(colors_.outline);
    outline.setWidth(2);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(outline);
    
    const unsigned int epoch = timeline_.last().timestamp;

    QVector<QPoint> points;
    // first point is at the bottom right corner (time values grom from left to right)
    // but we paint from right to left
    points.push_back(QPoint(width, height));

    for (int i=timeline_.size()-1; i>=0; --i)
    {
        const sample& s = timeline_[i];
            
        double t = epoch - s.timestamp;
        int x = width - ((t / 1000.0) * PIXELS_PER_SECOND);      
        int y = height - (int)((s.value / double(maxval_)) * height);
        QPoint p(x, y);
        points.append(p);
    }

    Q_ASSERT(!points.isEmpty());
           
    // the ending polygon is on the same x-axis as the last graph point
    // except that its aligned on the y axis with the first point 
    const QPoint& last_graph_point = points.last();
    
    points.append(QPoint(last_graph_point.x(), height));

    // polygon is filled with currently selected brush.
    // create brush for background filling using the fill_ color
    QBrush fill(colors_.fill);
    p.setBrush(fill);
    p.drawPolygon(&points[0], static_cast<int>(points.size()));


    // this code only draws the graph outline
    /*
    QPoint px;
    for (int i=timeline_.size()-1; i>=0; --i)
    {
        const sample& s = timeline_[i];

        double t = epoch - s.timestamp;
        int x = width - ((t / 1000.0) * PIXELS_PER_SECOND);
        int y = height - (int)((s.value / (double)maxval_) * height);
        if (x < 0)
            break;
        QPoint px1(x, y);
        if (!px.isNull())
            p.drawLine(px, px1);
        px = px1;
    }
    */
}

    /*
void TinyGraph::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu;

    connect(menu.addAction("grad1"), SIGNAL(triggered(bool)), this, SLOT(action_choose_1st_gradient()));
    connect(menu.addAction("grad2"), SIGNAL(triggered(bool)), this, SLOT(action_choose_2nd_gradient()));
    connect(menu.addAction("fill"),  SIGNAL(triggered(bool)), this, SLOT(action_choose_fill_color()));
    connect(menu.addAction("line"),  SIGNAL(triggered(bool)), this, SLOT(action_choose_outline_color()));
    connect(menu.addAction("font"),  SIGNAL(triggered(bool)), this, SLOT(action_choose_font()));
    menu.exec(event->globalPos());
}
    */

void TinyGraph::action_choose_1st_gradient()
{
    QColor c = QColorDialog::getColor(colors_.grad1);
    if (c.isValid())
        colors_.grad1 = c;
    
    update();
}

void TinyGraph::action_choose_2nd_gradient()
{
    QColor c = QColorDialog::getColor(colors_.grad2);
    if (c.isValid())
        colors_.grad2 = c;
    
    update();
}

void TinyGraph::action_choose_outline_color()
{
    QColor c = QColorDialog::getColor(colors_.outline);
    if (c.isValid())
        colors_.outline = c;
    
    update();
}

void TinyGraph::action_choose_fill_color()
{
    QColor c = QColorDialog::getColor(colors_.fill);
    if (c.isValid())
        colors_.fill = c;
    
    update();
}

void TinyGraph::action_choose_font()
{
    font_ = QFontDialog::getFont(NULL, this);
    
    update();
}


} // app
