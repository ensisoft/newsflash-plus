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

#include <QObject>
#include <QVariant>
#include <functional>

namespace test
{
    // bridge a qt slot to a std::function
    class qt_slot_to_function  : public QObject
    {
        Q_OBJECT
    public:
        std::function<void (void)> on_settings_update;
        std::function<void (const char*, const QVariant&)> on_save_setting;
        std::function<void (const char*, QVariant&)> on_load_setting;

    public slots:
        void settings_update() 
        {
            if (on_settings_update)
                on_settings_update();
        }

        void save_setting(const char* name, const QVariant& var)
        {
            if (on_save_setting)
                on_save_setting(name, var);
        }

        void load_setting(const char* name, QVariant& var)
        {
            if (on_load_setting)
                on_load_setting(name, var);
        }

    private:
    };

} // test