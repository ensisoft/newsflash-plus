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

} // namespace

namespace app
{

class ParState::State 
{
public:
    virtual ~State() = default;

    // returns true if there was a state update.
    virtual bool update(const QString& line, ParityChecker::File& out) = 0;

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
    virtual bool update(const QString& line, ParityChecker::File& file) override
    {
        QStringList captures;
        if (isMatch(line, "^Loading: \"(.+\\.par2)\"", captures, 1))
        {
            file.name  = captures[0];
            file.type  = ParityChecker::FileType::Parity;
            file.state = ParityChecker::FileState::Loading;
        }
        else if (isMatch(line, "^Loaded: \"(.+\\.par2)\"", captures, 1))
        {
            file.name  = captures[0];
            file.state = ParityChecker::FileState::Loaded;
            file.type  = ParityChecker::FileType::Parity;
        }
        else return false;

        return true;
    }
private:
};

// verify the archieve files (called targets)
class ParState::VerifyingState : public State
{
public:
    virtual bool update(const QString& line, ParityChecker::File& file)
    {
        QStringList captures;
        if (isMatch(line, "^Scanning: \"(.+)\"", captures, 1))
        {
            file.name  = captures[0];
            file.state = ParityChecker::FileState::Scanning;
            file.type  = ParityChecker::FileType::Target;
        }        
        else if (isMatch(line, "^Target: \"(.+)\" - (\\w+)\\.", captures, 2))
        {
            file.name  = captures[0];
            file.type  = ParityChecker::FileType::Target;
            file.state = ParityChecker::FileState::Scanning;
            if (captures[1] == "damaged")
                file.state = ParityChecker::FileState::Damaged;       
            else if (captures[1] == "found")     
                file.state = ParityChecker::FileState::Complete;
            else if (captures[1] == "missing")
                file.state = ParityChecker::FileState::Missing;
            else if (captures[1] == "empty")
                file.state = ParityChecker::FileState::Empty;
        } 
        else return false;

        return true;
    }
private:
};

// scan extra files
class ParState::ScanningState : public State
{
public:
    virtual bool update(const QString& line, ParityChecker::File& file)
    {
        QStringList captures;
        if (isMatch(line, "^File: \"(.+)\" - is a match for \"(.+)\"", captures, 2))
        {
            file.name  = captures[1];
            file.state = ParityChecker::FileState::Found;
            return true;
        }    

        return false;
    }
private:
};

class ParState::RepairState : public State
{
public:
    virtual bool update(const QString& line, ParityChecker::File&) override
    { return false; }

    virtual ExecState getState() const override
    { return ExecState::Repair; }
private:
};

class ParState::TerminalState : public State
{
public:
    TerminalState(const QString& message, bool success) : message_(message), success_(success)
    {}

    virtual bool update(const QString& line, ParityChecker::File&) override
    { return false; }

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

// have to define ctor and dtor here because of the
// private implementation.
ParState::ParState() : state_(new LoadingState)
{}

ParState::~ParState()
{}

void ParState::clear()
{
    state_.reset(new LoadingState);
}

bool ParState::update(const QString& line, ParityChecker::File& file)
{
    if (line.isEmpty())
        return false;

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

    return state_->update(line, file);
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

bool ParState::parseScanProgress(const QString& line, QString& file, float& done)
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
