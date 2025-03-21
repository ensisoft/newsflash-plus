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
#  include "ui_files.h"
#include "newsflash/warnpop.h"
#include "mainwidget.h"
#include "finder.h"

namespace app {
    class Files;
} // app

namespace gui
{
    // downloaded files GUI
    class Files : public MainWidget, public Finder
    {
        Q_OBJECT

    public:
        Files(app::Files& files);
       ~Files();

        // MainWidget implementation
        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void loadState(app::Settings& s) override;
        virtual void saveState(app::Settings& s) override;
        virtual void shutdown() override;
        virtual void refresh(bool isActive) override;
        virtual void activate(QWidget*) override;
        virtual info getInformation() const override;
        virtual Finder* getFinder() override;

        // Finder implementation
        virtual bool isMatch(const QString& str, std::size_t index, bool caseSensitive) override;
        virtual bool isMatch(const QRegExp& regex, std::size_t index) override;
        virtual std::size_t numItems() const override;
        virtual std::size_t curItem() const override;
        virtual void setFound(std::size_t index) override;

    private slots:
        void on_actionOpenFile_triggered();
        void on_actionOpenFileWith_triggered();
        void on_actionClear_triggered();
        void on_actionOpenFolder_triggered();
        void on_actionDelete_triggered();
        void on_actionForget_triggered();
        void on_tableFiles_customContextMenuRequested(QPoint point);
        void on_tableFiles_doubleClicked();
        void on_chkKeepSorted_clicked();
        void invokeTool();
        void toolsUpdated();

    private:
        Ui::Files mUi;
    private:
        app::Files& mModel;
        std::size_t mNumFiles = 0;
    };
} // gui