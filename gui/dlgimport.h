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
#  include <QtGui/QDialog>
#  include <QObject>
#  include "ui_dlgimport.h"
#include "newsflash/warnpop.h"

#include <vector>

#include "app/newznab.h"
#include "app/webquery.h"

namespace gui
{
    class DlgImport : public QDialog
    {
        Q_OBJECT

    public:
        DlgImport(QWidget* parent, std::vector<app::newznab::Account>& accs);
       ~DlgImport();

    private slots:
        void on_btnStart_clicked();
        void on_btnClose_clicked();
    private:
        void registerNext(QString email, std::size_t index);
        void registerInfo(const app::newznab::HostInfo& ret, QString email, std::size_t index);

    private:
        Ui::Import m_ui;
    private:
        std::vector<app::newznab::Account>& m_accounts;
    private:
        app::WebQuery* m_query = nullptr;
    };

} // gui