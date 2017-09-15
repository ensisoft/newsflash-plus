// Copyright (c) 2010-2017 Sami Väisänen, Ensisoft
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
#  include <QObject>
#  include "ui_filesystem.h"
#include "newsflash/warnpop.h"

#include "mainmodule.h"
#include "settings.h"

namespace gui
{
    class FileSystemSettings : public SettingsWidget
    {
        Q_OBJECT

    public:
        FileSystemSettings();
       ~FileSystemSettings();

        virtual bool validate() const override;

    private slots:
        void on_btnAdult_clicked();
        void on_btnApps_clicked();
        void on_btnMusic_clicked();
        void on_btnGames_clicked();
        void on_btnImages_clicked();
        void on_btnMovies_clicked();
        void on_btnTelevision_clicked();
        void on_btnOther_clicked();
        void on_btnLog_clicked();

    private:
        Ui::FileSystem mUI;
    private:
        friend class FileSystemModule;
    };

    class FileSystemModule : public QObject, public MainModule
    {
        Q_OBJECT

    public:
        FileSystemModule();
       ~FileSystemModule();

        virtual void loadState(app::Settings& s) override;
        virtual void saveState(app::Settings& s) override;
        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* gui) override;
        virtual void freeSettings(SettingsWidget* gui) override;

        virtual info getInformation() const override
        { return {"FileSystemModule", ""}; }
    private:

    };

} // namespace
