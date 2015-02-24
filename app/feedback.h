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
#  include <QByteArray>
#include <newsflash/warnpop.h>
#include <functional>
#include "netman.h"

namespace app
{
    class Feedback
    {
    public:
        enum class type {
            feedback,
            bugreport,
            request_feature,
            request_license
        };
        enum class feeling {
            positive, neutral, negative
        };

        enum class response {
            success = 0,
            dirty_rotten_spammer = 1,
            database_unavailable = 2,
            database_error = 3, 
            email_unavailable = 4,
            network_error = 5            
        };

        std::function<void (response r)> on_complete;

        Feedback();
       ~Feedback();

        struct Message {
            Feedback::type type;
            Feedback::feeling feeling;

            QString name;
            QString email;
            QString country;
            QString platform;
            QString text;
            QString attachment;
            QString version;
        };

        void send(const Message& m);

    private:
        NetworkManager::Context net_;
    };

} // app