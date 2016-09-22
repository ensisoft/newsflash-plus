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

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QtGui/QWidget> 
#include "newsflash/warnpop.h"

namespace gui
{
     class PieView : public QWidget
     {
          Q_OBJECT

     public:
          PieView(QWidget *parent = 0);

          // should be between 0.0 and 1.0
          void setAvailSlice(double value)
          { 
               m_avail = value;
               update();
          }
          void setDrawPie(bool onOff)
          { m_drawPie = onOff; }

          void setScale(double scale)
          { m_scale = scale; }

     private:
          virtual void paintEvent(QPaintEvent* event) override;
          

     private:
          double m_avail;
          double m_scale;
          bool m_drawPie;
     };

} // gui
