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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtGui/QLineEdit>
#include "newsflash/warnpop.h"

#include <deque>

#include "engine/ui/task.h"
#include "dlgselectaccount.h"
#include "dlgaccount.h"
#include "dlgduplicate.h"
#include "dlglowdisk.h"
#include "app/accounts.h"
#include "app/historydb.h"
#include "app/platform.h"
#include "app/engine.h"
#include "app/debug.h"

namespace gui
{

bool needAccountUserInput()
{
    if (app::g_accounts->numAccounts() == 0)
        return true;

    auto* main = app::g_accounts->getMainAccount();
    if (!main)
        return true;

    return false;
}

quint32 selectAccount(QWidget* parent, const QString& desc)
{
    if (app::g_accounts->numAccounts() == 0)
    {
        auto account = app::g_accounts->suggestAccount();
        DlgAccount dlg(parent, account, true);
        if (dlg.exec() == QDialog::Rejected)
            return 0;

        app::g_accounts->setAccount(account);
        return account.id;
    }

    auto* main = app::g_accounts->getMainAccount();
    if (main)
        return main->id;

    QStringList names;

    const auto num_acc = app::g_accounts->numAccounts();
    for (std::size_t i=0; i<num_acc; ++i)
    {
        const auto& acc = app::g_accounts->getAccount(i);
        names << acc.name;
    }
    DlgSelectAccount dlg(parent, names, desc);
    if (dlg.exec() == QDialog::Rejected)
        return 0;

    const auto& acc_name = dlg.account();

    int account_index;
    for (account_index=0; account_index<names.size(); ++account_index)
    {
        if (names[account_index] == acc_name)
            break;
    }
    const auto& acc = app::g_accounts->getAccount(account_index);

    if (dlg.remember())
        app::g_accounts->setMainAccount(acc.id);

    return acc.id;

}

bool isDuplicate(const QString& desc, app::MediaType type)
{
    return app::g_history->isDuplicate(desc, type);
}

bool passDuplicateCheck(QWidget* parent, const QString& desc,
    app::MediaType type)
{
    app::HistoryDb::Item item;
    if (!app::g_history->isDuplicate(desc, type, &item))
        return true;

    DlgDuplicate dlg(parent, desc, item);
    if (dlg.exec() == QDialog::Rejected)
        return false; // canceled

    const bool onOff = dlg.checkDuplicates();

    app::g_history->checkDuplicates(onOff);

    return true;
}

bool isDuplicate(const QString& desc)
{
    return app::g_history->isDuplicate(desc);
}

bool passDuplicateCheck(QWidget* parent, const QString& desc)
{
    app::HistoryDb::Item item;
    if (!app::g_history->isDuplicate(desc, &item))
        return true;

    DlgDuplicate dlg(parent, desc, item);
    if (dlg.exec() == QDialog::Rejected)
        return false; // canceled

    const bool onOff = dlg.checkDuplicates();

    app::g_history->checkDuplicates(onOff);

    return true;
}


bool passSpaceCheck(QWidget* parent,
    const QString& downloadDesc,
    const QString& downloadPath,
    quint64 expectedFinalBinarySize,
    quint64 expectedBatchSize,
    app::MediaType mediatype)
{
    if (!app::g_engine->getCheckLowDisk())
        return true;

    const auto& location   = app::g_engine->resolveDownloadPath(downloadPath, app::toMainType(mediatype));
    const auto& mountPoint = app::resolveMountPoint(location);
    const auto freeSpace   = app::getFreeDiskSpace(mountPoint);
    const auto queuedBytes = app::g_engine->getBytesQueued(mountPoint);

    const auto totalLoad = queuedBytes +
        expectedFinalBinarySize +
        expectedBatchSize;

    DEBUG("Disk partition at %1 has %2 free and currently has %3 queued",
        mountPoint, app::size { freeSpace },
        app::size { queuedBytes });
    DEBUG("Download requires approx. %2 ", app::size { expectedFinalBinarySize +
        expectedBatchSize });


    if (freeSpace > totalLoad)
        return true;

    DlgLowDisk dlg(parent, downloadDesc, mountPoint,
        queuedBytes,
        expectedFinalBinarySize + expectedBatchSize,
        freeSpace);
    if (dlg.exec() == QDialog::Rejected)
        return false; // canceled

    const bool onOff = dlg.checkLowDisk();

    app::g_engine->setCheckLowDisk(onOff);

    return true;
}

bool validate(QLineEdit* edit)
{
    const auto& text = edit->text();
    if (text.isEmpty())
    {
        edit->setFocus();
        return false;
    }
    return true;
}

} // gui
