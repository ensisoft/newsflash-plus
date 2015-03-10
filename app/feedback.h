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
#  include <QString>
#include <newsflash/warnpop.h>
#include <functional>

namespace app
{
    class Feedback
    {
    public:
        enum class Type {
            Feedback,
            BugReport,
            FeatureRequest,
            LicenseRequest
        };
        enum class Feeling {
            Positive, Neutral, Negative
        };

        enum class Response {
            Success = 0,
            DirtyRottenSpammer = 1,
            DatabaseUnavailable = 2,
            DatabaseError = 3, 
            EmailUnavailable = 4,
            NetworkError = 5            
        };

        std::function<void (Response r)> onComplete;

        Feedback();
       ~Feedback();

        void setType(Type type) 
        { type_ = type; }

        void setFeeling(Feeling feeling)
        { feeling_ = feeling; }

        void setName(QString name)
        { name_ = name; }

        void setEmail(QString email)
        { email_ = email; }

        void setCountry(QString country)
        { country_ = country; }

        void setPlatform(QString platform)
        { platform_ = platform; }

        void setVersion(QString version)
        { version_ = version; }

        void setMessage(QString text)
        { text_ = text; }

        void setAttachment(QString attachment)
        { attachment_ = attachment; }

        QString version() const 
        { return version_; }

        QString platform() const 
        { return platform_; }

        void send();

    private:
       Type type_;
       Feeling feeling_;    
    private:
       QString name_;
       QString email_;
       QString country_;
       QString platform_;
       QString text_;
       QString attachment_;
       QString version_;
    };

} // app