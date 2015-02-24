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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QString>
#include <newsflash/warnpop.h>
#include <memory>
#include "paritychecker.h"

namespace app
{
    // par2 command state parser
    class ParState
    {
    public:
        enum class ExecState {
            Scan, Repair, Finish
        };

        ParState();
       ~ParState();

        // clear existing state.
        void clear();

        // update state from a line of input.
        // returns true if there's new state. the new state
        // will be stored in the file object.
        bool update(const QString& line, ParityChecker::File& file);

        // get final success/failure value. 
        // only valid once ExecState reaches Finish.
        bool getSuccess() const;

        // get current message (if any)
        QString getMessage() const;

        // get current execution state.
        ExecState getState() const;
 
        static 
        bool parseScanProgress(const QString& line, QString& file, float& done);

        static 
        bool parseRepairProgress(const QString& line, QString& step, float& done);

    private:
        class State;
        class LoadingState;
        class VerifyingState;
        class ScanningState;
        class RepairState;
        class TerminalState;
        std::unique_ptr<State> state_;

    private:
    };

} // app