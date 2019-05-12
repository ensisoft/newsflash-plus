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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtGui/QWidget>
#  include <QString>
#  include <QObject>
#include "newsflash/warnpop.h"

#include <vector>

class QMenu;
class QToolBar;

namespace app {
    class Settings;
} // app

namespace gui
{
    class MainWidget;
    class SettingsWidget;
    class Finder;

    enum class WidgetType {
        Nzb,
        Rss,
        Search
    };

    namespace detail {
        class WidgetFactoryBase
        {
        public:
            virtual ~WidgetFactoryBase() = default;
            WidgetFactoryBase(WidgetType type)
              : mType(type)
              {}
            MainWidget* createWidget(WidgetType type) const
            {
                if (type != mType)
                    return nullptr;
                return createWidget();
            }
        protected:
            virtual gui::MainWidget* createWidget() const = 0;
        private:
            const WidgetType mType;
        };

        // it should be possible to replace the widget factory templates
        // below with a single widget factory taking variadic template arguments
        // and packing them into a tuple and using std::apply (c++17) to
        // pass the variables to the widget ctor. However that'd require
        // c++17 which means having to upgrade the tool chain which is a
        // lot of work. using the conventional / manual method for now.

        template<typename WidgetT>
        class WidgetFactory_arg0 : public WidgetFactoryBase
        {
        public:
            WidgetFactory_arg0(WidgetType type)
              : WidgetFactoryBase(type)
            {}

            virtual gui::MainWidget* createWidget() const override
            { return new WidgetT(); }
        private:
        };
        template<typename WidgetT, typename Arg0>
        class WidgetFactory_arg1 : public WidgetFactoryBase
        {
        public:
            WidgetFactory_arg1(WidgetType type, const Arg0& a)
              : WidgetFactoryBase(type)
              , mArg(a)
            {}

            virtual gui::MainWidget* createWidget() const override
            { return new WidgetT(mArg); }
        private:
            const Arg0 mArg;
        };
        template<typename WidgetT, typename Arg0, typename Arg1>
        class WidgetFactory_arg2 : public WidgetFactoryBase
        {
        public:
            WidgetFactory_arg2(WidgetType type, const Arg0& a0, const Arg1& a1)
              : WidgetFactoryBase(type)
              , mArg0(a0)
              , mArg1(a1)
            {}

            virtual gui::MainWidget* createWidget() const override
            { return new WidgetT(mArg0, mArg1); }
        private:
            const Arg0 mArg0;
            const Arg1 mArg1;
        };

    } // namespace

    // mainwidget objects sit in the mainwindow's main tab
    // and provides GUI and features to the application.
    // the different between mainwidget and mainmodule is that
    // mainmodules are simpler headless versions that no not provide
    // a user visible GUI
    class MainWidget : public QWidget
    {
        Q_OBJECT

    public:
        struct info {
            // this is the URL to the help (file). If it specifies a filename
            // it is considered to be a help file in the application's help installation
            // folder. Otherwise an absolute path or URL can be specified.
            QString helpurl;

            // Whether the component should be visible by default (on first launch).
            bool initiallyVisible;
        };

        virtual ~MainWidget() = default;

        // Add the component specific menu actions to a menu inside the host application
        virtual void addActions(QMenu& menu) {}

        // Add the component specific toolbar actions to a toolbar in the host application
        virtual void addActions(QToolBar& bar) {}

        // This function is invoked when this ui component is getting activated (becomes visible)
        // in the host GUI.
        virtual void activate(QWidget* parent) {}

        // This function is invoked when this ui component is hidden in the host GUI.
        virtual void deactivate() {}

        // load the widget/component state on application startup
        virtual void loadState(app::Settings& s) {}

        // save the widget/component state.
        // can throw an exception.
        virtual void saveState(app::Settings& s) {};

        // prepare the widget/component for shutdown
        virtual void shutdown() {}

        // startup up the widget after application has loaded
        // all the settings & data.
        virtual void startup() {}

        // refresh the widget contents
        virtual void refresh(bool isActive) {}

        // perform first launch activities.
        virtual void firstLaunch() {}

        // get information about the widget/component.
        virtual info getInformation() const { return {"", false}; }

        // get a settings widget if any. the object ownership is
        // transferred to the caller.
        virtual SettingsWidget* getSettings() { return nullptr; }

        virtual void applySettings(SettingsWidget* gui) {}

        // notify that application settings have changed.
        virtual void freeSettings(SettingsWidget* s) {}

        virtual Finder* getFinder() { return nullptr; }

        virtual void updateRegistration(bool success) {};

        // consume the given file on drag & drop action.
        // returns true if the file was accepted or false
        // if the file was not supported/accepted by this
        // mainwidget instance.
        virtual bool dropFile(const QString& name) { return false; }

        // open the given file.
        // returns true if the file was supported and opened
        // or false if the file was not supported by this
        // mainwidget instance.
        virtual bool openFile(const QString& name) { return false; }

        static MainWidget* createWidget(WidgetType type)
        {
            const auto& WidgetFactories = GetWidgetFactories();
            for (const auto* factory : WidgetFactories)
            {
                MainWidget* widget = factory->createWidget(type);
                if (widget)
                    return widget;
            }
            return nullptr;
        }

        static MainWidget* createSearchWidget()
        { return createWidget(WidgetType::Search); }

        static MainWidget* createRssWidget()
        { return createWidget(WidgetType::Rss); }

        static MainWidget* createNzbWidget()
        { return createWidget(WidgetType::Nzb); }

        template<typename T> static
        void registerWidgetType(WidgetType type)
        {
            GetWidgetFactories().emplace_back(new detail::WidgetFactory_arg0<T>(type));
        }
        template<typename T, typename Arg0> static
        void registerWidgetType(WidgetType type, const Arg0& a0)
        {
            GetWidgetFactories().emplace_back(new detail::WidgetFactory_arg1<T, Arg0>(type, a0));
        }
        template<typename T, typename Arg0, typename Arg1> static
        void registerWidgetType(WidgetType type, const Arg0& a0, const Arg1& a1)
        {
            GetWidgetFactories().emplace_back(new detail::WidgetFactory_arg2<T, Arg0, Arg1>(type, a0, a1));
        }

    signals:
        // request the widget to have it's toolbar updated.
        void updateMenu(MainWidget* self);

        // request the settings to be displayed.
        void showSettings(const QString& tabName);

    private:
        static std::vector<detail::WidgetFactoryBase*>& GetWidgetFactories()
        {
            static std::vector<detail::WidgetFactoryBase*> WidgetFactories;
            return WidgetFactories;
        }
    };

} // gui
