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
#  include "ui_nzbfile.h"
#include "newsflash/warnpop.h"

#include <memory>

#include "app/nzbfile.h"
#include "app/media.h"
#include "mainwidget.h"
#include "finder.h"

namespace gui
{
    // nzbfile widget displays the contents of a single NZB file
    class NZBFile : public MainWidget, public Finder
    {
        Q_OBJECT

    public:
        NZBFile();
        NZBFile(app::MediaType type);
       ~NZBFile();

        // MainWidget implementation
        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void loadState(app::Settings& s) override;
        virtual void saveState(app::Settings& s) override;
        virtual info getInformation() const override;
        virtual Finder* getFinder() override;

        // Finder
        virtual bool isMatch(const QString& str, std::size_t index, bool caseSensitive) override;
        virtual bool isMatch(const QRegExp& reg, std::size_t index) override;
        virtual std::size_t numItems() const override;
        virtual std::size_t curItem() const override;
        virtual void setFound(std::size_t index) override;


        // open and display the contents of the given NZB file.
        void open(const QString& nzbfile);

        // open and display the contents of the given in memory buffer.
        void open(const QByteArray& nzb, const QString& desc);

    private slots:
        void on_actionDownload_triggered();
        void on_actionBrowse_triggered();
        void on_tableView_customContextMenuRequested(QPoint);
        void on_chkFilenamesOnly_clicked();
        void on_btnSelectAll_clicked();
        void on_btnSelectNone_clicked();
        void downloadToPrevious();
    private:
        void downloadSelected(const QString& folder);

    private:
        Ui::NZB mUi;
        app::NZBFile mModel;
    };

} // gui
