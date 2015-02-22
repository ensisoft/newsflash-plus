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
#include <vector>
#include <functional>

namespace app
{
    // par2 command state parser
    class ParState
    {
    public:
        enum class ExecState {
            Scan, Repair, Finish
        };

        // current file status
        enum class FileState {
            // for par2 files 
            Loading, 

            Loaded,

            // for data files
            // Scanning the contents of the file
            Scanning, 

            // the file is missing.
            Missing,

            Found,

            Empty,

            //NoDataFound,

            // the file is damaged
            Damaged,

            // file is complete
            Complete
        };

        struct File  {
            FileState  state;
            QString    file;
        };

        std::function<void(void)> onInsert;

        ParState(std::vector<File>& state);
       ~ParState();

        void clear();

        void update(const QString& line);

        bool getSuccess() const;

        QString getMessage() const;

        ExecState getState() const;
 
        static 
        bool parseFileProgress(const QString& line, QString& file, float& done);

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
        std::vector<File>& files_;

    };

} // app