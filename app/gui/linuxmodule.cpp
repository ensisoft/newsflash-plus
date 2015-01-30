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

linuxsettings::linuxsettings()
{
    ui_.setupUi(this);
}

linuxsettings::~linuxsettings()
{}

bool linuxsettings::validate() const 
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
void linuxsettings::on_btnDefaultOpenfileCommand_clicked()
{
    ui_.editOpenfileCommand->setText("xdg-open");
}

void linuxsettings::on_btnDefaultShutdownCommand_clicked()
{
    ui_.editShutdownCommand->setText("systemctl poweroff");
}


linuxmodule::linuxmodule()
{}

linuxmodule::~linuxmodule()
{}

void linuxmodule::loadstate(app::settings& s)
{
#if defined(LINUX_OS)
    auto shutdown = app::get_shutdown_command();
    auto openfile = app::get_openfile_command();

    shutdown = s.get("linux", "shutdown_command", shutdown);
    openfile = s.get("linux", "openfile_command", openfile);
    app::set_openfile_command(openfile);
    app::set_shutdown_command(shutdown);

    DEBUG(str("Openfile command set to _1", openfile));
    DEBUG(str("Shutdown command set to _1", shutdown));
#endif
}

bool linuxmodule::savestate(app::settings& s)
{
#if defined(LINUX_OS)
    const auto shutdown = app::get_shutdown_command();
    const auto openfile = app::get_openfile_command();

    s.set("linux", "shutdown_command", shutdown);
    s.set("linux", "openfile_command", openfile);
#endif
    return true;
}

gui::settings* linuxmodule::get_settings() 
{
    auto* ptr = new linuxsettings;
    auto& gui = ptr->ui_;

#if defined(LINUX_OS)
    const auto shutdown = app::get_shutdown_command();
    const auto openfile = app::get_openfile_command();

    gui.editOpenfileCommand->setText(openfile);
    gui.editShutdownCommand->setText(shutdown);
#endif

    return ptr;
}

void linuxmodule::apply_settings(settings* s)
{
    auto* ptr = dynamic_cast<linuxsettings*>(s);
    auto& gui = ptr->ui_;

#if defined(LINUX_OS)
    const auto shutdown = gui.editShutdownCommand->text();
    const auto openfile = gui.editOpenfileCommand->text();

    app::set_openfile_command(openfile);
    app::set_shutdown_command(shutdown);
#endif
}

void linuxmodule::free_settings(settings* s)
{
    delete s;
}

} // gui

