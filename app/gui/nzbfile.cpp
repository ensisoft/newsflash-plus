// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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

#define LOGTAG "nzb"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QMessageBox>
#include <newsflash/warnpop.h>

#include "nzbfile.h"
#include "mainwindow.h"
#include "../debug.h"
#include "../format.h"
#include "../eventlog.h"
#include "../settings.h"

namespace gui
{
nzbfile::nzbfile()
{
    ui_.setupUi(this);
    ui_.progressBar->setVisible(false);
    ui_.progressBar->setValue(0);
    ui_.progressBar->setRange(0, 0);
    ui_.tableView->setModel(&model_);

    model_.on_ready = [&]() {
        ui_.progressBar->setVisible(false);
    };
    DEBUG("nzbfile created");
}

nzbfile::~nzbfile()
{
    DEBUG("nzbfile deleted");
}

void nzbfile::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionOpen);
    menu.addSeparator();
    menu.addAction(ui_.actionDownload);
    menu.addSeparator();
    menu.addAction(ui_.actionClear);
    menu.addSeparator();
    menu.addAction(ui_.actionDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionSettings);
}

void nzbfile::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionOpen);
    bar.addSeparator();
    bar.addAction(ui_.actionDownload);
    bar.addSeparator();
    bar.addAction(ui_.actionClear);
    bar.addSeparator();
    bar.addAction(ui_.actionDelete);
    bar.addSeparator();
    bar.addAction(ui_.actionSettings);
}

void nzbfile::close_widget()
{
    delete this;
}

void nzbfile::loadstate(app::settings& s)
{
    const bool filenames = s.get("nzb", "show_filename_only", false);

    ui_.chkFilenamesOnly->setChecked(filenames);

    model_.set_show_filenames_only(filenames);
}

bool nzbfile::savestate(app::settings& s)
{
    s.set("nzb", "show_filename_only", 
        ui_.chkFilenamesOnly->isChecked());
    return true;
}

void nzbfile::open(const QString& nzbfile)
{
    Q_ASSERT(!nzbfile.isEmpty());

    if (model_.load(nzbfile))
    {
        ui_.progressBar->setVisible(true);
        ui_.grpBox->setTitle(nzbfile);
    }
}

void nzbfile::open(const QByteArray& bytes, const QString& desc)
{
    Q_ASSERT(!bytes.isEmpty());
    Q_ASSERT(!desc.isEmpty());

    if (model_.load(bytes, desc))
    {
        ui_.progressBar->setVisible(true);
        ui_.grpBox->setTitle(desc);
    }
}

void nzbfile::ready()
{
    ui_.progressBar->setVisible(false);
}

void nzbfile::on_actionOpen_triggered()
{
    const auto& file = g_win->select_nzb_file();
    if (file.isEmpty())
        return;

    if (model_.load(file))
    {
        ui_.progressBar->setVisible(true);
        ui_.grpBox->setTitle(file);
    }
}

void nzbfile::on_actionDownload_triggered()
{}

void nzbfile::on_actionClear_triggered()
{
    model_.clear();
    ui_.grpBox->setTitle(tr("NZB File"));
    ui_.actionDownload->setEnabled(false);
    ui_.actionDelete->setEnabled(false);
}

void nzbfile::on_listFiles_customContextMenuRequested(QPoint)
{
    //QMenu sub("Download to");
    //sub.setIcon()

    //QMenu menu(this);
}

void nzbfile::on_chkFilenamesOnly_clicked()
{
    const bool on_off = ui_.chkFilenamesOnly->isChecked();

    model_.set_show_filenames_only(on_off);
}

} // gui


