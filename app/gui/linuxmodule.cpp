// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.         

#define LOGTAG "linux"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>

#include <newsflash/warnpop.h>

#include "linuxmodule.h"
#include "../platform.h"
#include "../settings.h"
#include "../debug.h"
#include "../format.h"

using app::str;

namespace gui
{

LinuxSettings::LinuxSettings()
{
    ui_.setupUi(this);
}

LinuxSettings::~LinuxSettings()
{}

bool LinuxSettings::validate() const 
{
    const auto& shutdown = ui_.editShutdownCommand->text();
    const auto& openfile = ui_.editOpenfileCommand->text();
    if (shutdown.isEmpty())
    {
        ui_.editShutdownCommand->setFocus();
        return false;
    }

    if (openfile.isEmpty())
    {
        ui_.editOpenfileCommand->setFocus();
        return false;
    }
    return true;

}
void LinuxSettings::on_btnDefaultOpenfileCommand_clicked()
{
    ui_.editOpenfileCommand->setText("xdg-open");
}

void LinuxSettings::on_btnDefaultShutdownCommand_clicked()
{
    ui_.editShutdownCommand->setText("systemctl poweroff");
}


LinuxModule::LinuxModule()
{}

LinuxModule::~LinuxModule()
{}

void LinuxModule::loadState(app::Settings& s)
{
#if defined(LINUX_OS)
    auto shutdown = app::getShutdownCommand();
    auto openfile = app::getOpenfileCommand();

    shutdown = s.get("linux", "shutdown_command", shutdown);
    openfile = s.get("linux", "openfile_command", openfile);
    app::setOpenfileCommand(openfile);
    app::setShutdownCommand(shutdown);

    DEBUG(str("Openfile command set to _1", openfile));
    DEBUG(str("Shutdown command set to _1", shutdown));
#endif
}

bool LinuxModule::saveState(app::Settings& s)
{
#if defined(LINUX_OS)
    const auto shutdown = app::getShutdownCommand();
    const auto openfile = app::getOpenfileCommand();

    s.set("linux", "shutdown_command", shutdown);
    s.set("linux", "openfile_command", openfile);
#endif
    return true;
}

SettingsWidget* LinuxModule::getSettings() 
{
    auto* ptr = new LinuxSettings;
    auto& gui = ptr->ui_;

#if defined(LINUX_OS)
    const auto shutdown = app::getShutdownCommand();
    const auto openfile = app::getOpenfileCommand();

    gui.editOpenfileCommand->setText(openfile);
    gui.editShutdownCommand->setText(shutdown);
#endif

    return ptr;
}

void LinuxModule::applySettings(SettingsWidget* s)
{
    auto* ptr = dynamic_cast<LinuxSettings*>(s);
    auto& gui = ptr->ui_;

#if defined(LINUX_OS)
    const auto shutdown = gui.editShutdownCommand->text();
    const auto openfile = gui.editOpenfileCommand->text();

    app::setOpenfileCommand(openfile);
    app::setShutdownCommand(shutdown);
#endif
}

void LinuxModule::freeSettings(SettingsWidget* s)
{
    delete s;
}

} // gui

