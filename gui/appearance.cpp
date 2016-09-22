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

#define LOGTAG "style"

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QtGui/QStyleFactory>
#  include <QFile>
#  include <QTextStream>
#  include <QString>
#  include "ui_appearance.h"
#include "newsflash/warnpop.h"
#include "appearance.h"
#include "app/format.h"
#include "app/settings.h"
#include "app/homedir.h"
#include "app/debug.h"
#include "app/format.h"
#include "app/eventlog.h"

namespace {

    // default style means that no specific style is set in the application
    // and the style that applies is the system's Qt setting set in qt-config
    class MySettings : public gui::SettingsWidget
    {
    public:
        MySettings(const QString& current_style)
        {
            ui_.setupUi(this);

            const auto styles = QStyleFactory::keys();
            for (int i=0; i<styles.size(); ++i)
            {
                QRadioButton* rd = new QRadioButton(styles[i], 
                    ui_.grpStyle);
                if (current_style == styles[i])
                    rd->setChecked(true);
                ui_.verticalLayout->addWidget(rd);
            }
            if (current_style == "Default")
                ui_.radioDefaultStyle->setChecked(true);
        }
       ~MySettings()
        {}

        QString getStyleName() const
        {
            // find the style that was checked
            const auto& children = ui_.grpStyle->children();
            for (int i=0; i<children.size(); ++i)
            {
                QObject* child = children[i];
                if (QRadioButton* btn = qobject_cast<QRadioButton*>(child))
                {
                    if (btn->isChecked())
                        return btn->text();
                }
            }
            Q_ASSERT(!"wut");
            return "";
        }
    private:
        Ui::Appearance ui_;
    };
} // namespace

namespace gui
{

Appearance::Appearance() : current_style_name_("Default")
{}

Appearance::~Appearance()
{}

void Appearance::loadState(app::Settings& s)
{
    current_style_name_ = s.get("theme", "name", "Default");
    if (current_style_name_ == "Default")
        return;

    QStyle* style = QApplication::setStyle(current_style_name_);
    if (!style)
    {
        WARN("No such style %1", current_style_name_);
        WARN("Style set to Default");
        current_style_name_ = "Default";
        return;
    }
    QApplication::setPalette(style->standardPalette());

    DEBUG("Qt style %1", current_style_name_);
}

void Appearance::saveState(app::Settings& s)
{
    s.set("theme", "name", current_style_name_);
}

SettingsWidget* Appearance::getSettings()
{
    return new MySettings(current_style_name_);
}

void Appearance::applySettings(SettingsWidget* gui) 
{
    const auto mine = dynamic_cast<MySettings*>(gui);    
    const auto name = mine->getStyleName();

    current_style_name_ = name;

    if (current_style_name_ == "Default")
        return;

    QStyle* style = QApplication::setStyle(current_style_name_);
    QApplication::setPalette(style->standardPalette());
}

void Appearance::freeSettings(SettingsWidget* s)
{
    delete s;
}



} // gui
