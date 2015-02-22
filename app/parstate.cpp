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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QRegExp>
#  include <QStringList>
#  include <QtDebug>
#include <newsflash/warnpop.h>
#include <newsflash/engine/assert.h>
#include "parstate.h"

namespace {

bool isMatch(const QString& line, const QString& test, QStringList& captures, int numCaptures)
{
    QRegExp exp;
    exp.setPattern(test);
    exp.setCaseSensitivity(Qt::CaseInsensitive);
    Q_ASSERT(exp.isValid());

    if (exp.indexIn(line) == -1)
        return false;

    Q_ASSERT(exp.captureCount() == numCaptures);

    // cap(0) is the whole regexp

    captures.clear();
    for (int i=0; i<numCaptures; ++i)
    {
        captures << exp.cap(i+1);
        //qDebug() << exp.cap(i+1);
    }

    return true;
}

app::ParState::File& findLast(std::vector<app::ParState::File>& files)
{
    Q_ASSERT(files.empty() == false);

    return files.back();
}

app::ParState::File& makeFile(const QString& file, app::ParState::FileState status, 
    std::vector<app::ParState::File>& files)
{
    auto it = std::find_if(files.rbegin(), files.rend(),
        [&](const app::ParState::File& f) {
            return f.file == file;
        });
    if (it == files.rend())
    {
        app::ParState::File data;
        data.state     = status;
        data.file      = file;
        files.push_back(data);
        return files.back();
    }
    return *it;
}

} // namespace

namespace app
{

class ParState::State 
{
public:
    virtual ~State() = default;

    virtual void update(const QString& line, std::vector<File>& files) = 0;

    virtual ExecState getState() const 
    { return ExecState::Scan; }

    virtual bool getSuccess() const 
    { return false; }

    virtual QString getMessage() const 
    { return {}; }
protected:
private:
};

// load par2 files
class ParState::LoadingState : public State
{
public:
    virtual void update(const QString& line, std::vector<File>& files) override
    {
        QStringList captures;
        if (isMatch(line, "^Loading: \"(.+\\.par2)\"", captures, 1))
        {
            makeFile(captures[0], FileState::Loading, files);
        }
        else if (isMatch(line, "^Loaded \\d+ new packets", captures, 0))
        {
            auto& par = findLast(files);
            par.state = FileState::Loaded;
        }
        else if (isMatch(line, "^No new packets found", captures, 0))
        {
            auto& par = findLast(files);
            par.state = FileState::Loaded;

        } 
    }
private:
};

// verify the archieve files (called targets)
class ParState::VerifyingState : public State
{
public:
    virtual void update(const QString& line, std::vector<File>& files)
    {
        QStringList captures;
        if (isMatch(line, "^Scanning: \"(.+)\"", captures, 1))
        {
            makeFile(captures[0], FileState::Scanning, files);
        }        
        else if (isMatch(line, "^Target: \"(.+)\" - (\\w+)\\.", captures, 2))
        {
            auto& data = makeFile(captures[0], FileState::Scanning, files);
            if (captures[1] == "damaged")
                data.state = FileState::Damaged;       
            else if (captures[1] == "found")     
                data.state = FileState::Complete;
            else if (captures[1] == "missing")
                data.state = FileState::Missing;
            else if (captures[1] == "empty")
                data.state = FileState::Empty;
        }    
    }
private:
};

// scan extra files
class ParState::ScanningState : public State
{
public:
    virtual void update(const QString& line, std::vector<File>& files)
    {
        QStringList captures;
        if (isMatch(line, "^File: \"(.+)\" - is a match for \"(.+)\"", captures, 2))
        {
            auto it = std::find_if(std::begin(files), std::end(files),
                [&](const File& f) {
                    return f.file == captures[1];
                });
            if (it != std::end(files))
            {
                auto& data = *it;
                data.state = FileState::Found;
            }
        }    
    }
private:
};

class ParState::RepairState : public State
{
public:
    virtual void update(const QString& line, std::vector<File>& files) override
    {}

    virtual ExecState getState() const override
    { return ExecState::Repair; }
private:
};

class ParState::TerminalState : public State
{
public:
    TerminalState(const QString& message, bool success) : message_(message), success_(success)
    {}
    virtual void update(const QString& line, std::vector<File>& files) override
    {}
    virtual ExecState getState() const override
    { return ExecState::Finish; }

    virtual bool getSuccess() const override
    { return success_; }

    virtual QString getMessage() const override
    { return message_; }
private:
    QString message_;
    bool success_;
};

ParState::ParState(std::vector<File>& state) : files_(state)
{
    clear();
}

ParState::~ParState()
{}

void ParState::clear()
{
    state_.reset(new LoadingState);
}

void ParState::update(const QString& line)
{
    if (line.isEmpty())
        return;

    QStringList captures;
    if (isMatch(line, "^Verifying source files:", captures, 0))
    {
        state_.reset(new VerifyingState);
    }
    else if (isMatch(line, "^Scanning extra files:", captures, 0))
    {
        state_.reset(new ScanningState);
    }
    else if (isMatch(line, "^Repair is required", captures, 0))
    {
        state_.reset(new RepairState);
    }
    else if (isMatch(line, "^Repair is not required", captures, 0))
    {
        state_.reset(new TerminalState(line, true));
    }
    else if (isMatch(line, "^Repair is not possible. \"(.+)\"", captures, 1))
    {
        state_.reset(new TerminalState(captures[0], false));
    }
    else if (isMatch(line, "^Repair failed", captures, 0))
    {
        state_.reset(new TerminalState(line, false));
    }
    else if (isMatch(line, "^Repair complete", captures, 0))
    {
        state_.reset(new TerminalState(line, true));
    }
    else
    {
        const auto numFiles = files_.size();
    
        state_->update(line, files_);

        if (numFiles != files_.size())
        {
            if (onInsert)
                onInsert();
        }
    }
}

bool ParState::getSuccess() const
{ 
    return state_->getSuccess();
}

QString ParState::getMessage() const
{
    return state_->getMessage();
}

ParState::ExecState ParState::getState() const
{
    return state_->getState();
}

bool ParState::parseFileProgress(const QString& line, QString& file, float& done)
{
    QStringList captures;
    if (isMatch(line, "^(Loading:|Scanning:) \"(.+)\": (\\d{2}\\.\\d)%", captures, 3))
    {
        file = captures[1];
        done = captures[2].toFloat();
        return true;
    }
    return false;
}

bool ParState::parseRepairProgress(const QString& line,  QString& step, float& done)
{
    QStringList captures;
    if (isMatch(line, "^((Constructing|Solving|Repairing).*): (\\d{2}.\\d)%", captures, 3))
    {
        step = captures[0];
        done = captures[2].toFloat();
        return true;
    }
    return false;
}

} // app
