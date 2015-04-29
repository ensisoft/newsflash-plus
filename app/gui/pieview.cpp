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
#  include <QtGui/QPainter>
#  include <QtGui/QBrush>
#  include <QtGui/QFont>
#  include <QtGui/QPen>
#  include <QtGui/QLinearGradient>
#  include <QRect>
#include <newsflash/warnpop.h>
#include "pieview.h"

namespace gui
{

PieView::PieView(QWidget* parent) : QWidget(parent), m_avail(1.0), m_scale(1.0), m_drawPie(true)
{}

void PieView::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::HighQualityAntialiasing);

    const auto rect   = this->rect();
    const auto width  = this->width();
    const auto height = this->height();
    const auto edge   = std::min(width, height) * m_scale;

    //QLinearGradient background(rect.topLeft(), rect.bottomRight());
    //background.setColorAt(0, QColor(255, 255, 255));
    //background.setColorAt(1, QColor(245, 245, 245));
    //painter.fillRect(rect, background);
    if (!m_drawPie)
        return;

    const auto y = (height - edge) / 2;
    const auto x = (width - edge) / 2; 

    const auto rc = QRect(x, y, edge, edge);

    QBrush used(Qt::darkRed);
    QBrush avail(Qt::darkGreen);    
    
    const auto sliceAvail = m_avail;
    const auto sliceUsed  = 1.0 - m_avail;
    const auto fullCircle = 360 * 16;

    painter.setBrush(avail);
    painter.drawPie(rc, 0, fullCircle);

    painter.setBrush(used);
    painter.drawPie(rc, 0, sliceUsed * fullCircle);
}

} // gui