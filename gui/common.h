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
#  include <QString>
#include "newsflash/warnpop.h"
#include "app/media.h"
// common UI functions.

class QWidget;
class QLineEdit;

namespace gui
{

bool needAccountUserInput();

// Prompts the user for account selection and returns
// the account id of the selected account or 0
// if the operation was canceled and no account selected.
quint32 selectAccount(QWidget* parent, const QString& desc);

bool isDuplicate(const QString& desc,
    app::MediaType);

bool isDuplicate(const QString& desc);

bool passDuplicateCheck(QWidget* parent, const QString& desc,
    app::MediaType type);

bool passDuplicateCheck(QWidget* parent, const QString& desc);

bool passSpaceCheck(QWidget* parent,
    const QString& desc,
    const QString& downloadPath,
    quint64 expectedFinalBinarySize,
    quint64 expectedBatchSize,
    app::MediaType mediatype);

enum class EditValidationType {
    AcceptString,
    AcceptNumber
};


bool validate(QLineEdit* edit,
    EditValidationType validateAs = EditValidationType::AcceptString);

} // gui
