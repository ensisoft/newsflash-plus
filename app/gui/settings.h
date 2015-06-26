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
#  include <QString>
#include <newsflash/warnpop.h>

namespace gui
{
    // settings widget extends the visible settings interface
    class SettingsWidget : public QWidget
    {
    public:
        virtual ~SettingsWidget() = default;

        // check if the settings are valid. if settings are not valid
        // a settings object can veto the accept() by returning false.
        // this prevents the dialog from closing.
        // otherwise if all the settings are ok then true should be returned.
        virtual bool validate() const { return true; };

        // apply was clicked.
        virtual void apply() {}

        // cancel was clicked
        virtual void cancel() {}

        // ok was clicked
        virtual void accept() {}
    protected:
    private:
    };

} // gui