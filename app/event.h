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
#  include <QMetaType>
#  include <QString>
#  include <QTime>
#include <newsflash/warnpop.h>

namespace app
{
    // application event
    struct Event {
        enum class Type {
            // useful information about an event that occurred.
            Info, 

            // like info except that it is transient and isnt logged.
            Note, 

            // warning means that things might not work quite as expected
            // but the particular processing can continue.
            // for example a downloaded file was damaged or some non-critical
            // file could not be opened.
            Warning, 

            // error means that some processing has encountered an unrecoverable
            // problem and probably cannot continue. For example connection was lost
            // critical file could not be read/written or some required
            // resource could not be acquired.
            Error
        };
        Type type;

        // event log message.
        QString message;

        // logtag identifies the component that generated the event.
        QString logtag;

        // recording time.
        QTime time;
    };

} // app

    Q_DECLARE_METATYPE(app::Event);
