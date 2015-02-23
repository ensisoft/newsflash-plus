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
#  include <QString>
#  include <QMetaType>
#include <newsflash/warnpop.h>

namespace app
{
    struct Recovery {
        enum class Status {
            // recovery is currently queued and will execute
            // at some later time.
            Queued,  

            // recovery is currently being performed.
            // recovery data will show the data
            // inside these parity files.
            Active, 

            // recovery was succesfully performed.
            Success, 

            // recovery failed
            Failed,

            // error running the recovery operation
            Error
        };

        // current recovery status.
        Status  state;

        // path to the recovery folder (where the par2 and data files are)
        QString path;

        // the main par2 file
        QString file; 

        // human readable description
        QString desc;

        // error/information message
        QString message;
    };

} // app

    Q_DECLARE_METATYPE(app::Recovery);