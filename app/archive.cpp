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
#include <newsflash/warnpop.h>
#include "archive.h"

namespace app
{

QString toString(Archive::Status status)
{
    using s = Archive::Status;
    switch (status)
    {
        case s::Queued:  return "Queued";
        case s::Active:  return "Active";
        case s::Success: return "Success";
        case s::Error:   return "Error";
        case s::Failed:  return "Failed";
    }
    Q_ASSERT(0);
    return {};
}

QIcon toIcon(Archive::Status status)
{
    using s = Archive::Status;
    switch (status)
    {
        case s::Queued:  return QIcon("icons:ico_recovery_queued.png");
        case s::Active:  return QIcon("icons:ico_recovery_active.png");
        case s::Success: return QIcon("icons:ico_recovery_success.png");
        case s::Error:   return QIcon("icons:ico_recovery_error.png");
        case s::Failed:  return QIcon("icons:ico_recovery_failed.png");
    }    
    Q_ASSERT(0);
    return {};
}

} // app