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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  if defined(WINDOWS_OS)
#    include <ActiveQt/QAxWidget>
#    define BASE_CLASS QAxWidget
#  else
#    include <QtGui/QWidget>
#    define BASE_CLASS QWidget
#  endif
#include "newsflash/warnpop.h"

namespace gui
{
    class WebWidget : public BASE_CLASS
    {
    public:
        WebWidget(QWidget* parent = nullptr) : BASE_CLASS(parent)
        {
        #if defined(WINDOWS_OS)
            // this magic here is a GUID. Not sure how to find one,
            // but it should be the COM class identifier for the
            // microsoft IE browser component.
            // this GUID was found in the Qt4's examples examples/activeqt/webbrowser
            // http://doc.qt.io/archives/qt-4.8/qaxwidget.html
            setControl("8856F961-340A-11D0-A96B-00C04FD705A2");
        #endif
        }
        void loadUrl(const QString& url)
        {
        #if defined(WINDOWS_OS)
            dynamicCall("Navigate(const QString&", url);
        #endif
        }

    protected:
    };

} // namespace
