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

#define LOGTAG "python"

#include <newsflash/config.h>

#if defined(NEWSFLASH_ENABLE_PYTHON)

#include <boost/noncopyable.hpp>

#include <newsflash/warnpush.h>
#  include <QDir>
#  include <QStringList>
#include <newsflash/warnpop.h>

#include "python.h"
#include "eventlog.h"
#include "format.h"
#include "distdir.h"
#include "debug.h"
#include "script.h"

namespace {

// host module extension functions
// remember that all these functions are invoked by the python
// so their interaction with the main application objects
// needs to be thread safe
PyObject* newsflash_add_log(PyObject* self, PyObject* args)
{
    return nullptr;
}

PyObject* newsflash_write_output(PyObject* self, PyObject* args)
{
    return nullptr;
}

PyObject* newsflash_system(PyObject* self, PyObject* args)
{
    return nullptr;
}

} // namespace

namespace app
{

python::python()
{
    INFO(str("Python _1", Py_GetVersion()));
    INFO("http://www.python.org");    

    // IMPORTANT:
    // We have to tell python the installation location of the application
    // so that it knows where to look for modules (both .dll and .py)
    // Unfortunately the API is based on char* which means that Unicode
    // paths cannot be represented (windows problem).
    static auto binary_name = distdir::path("newsflash").toLocal8Bit();
    static auto binary_path = distdir::path().toLocal8Bit();

    // initialize python
    // Embedding Python in Multi-Threaded C/C++ applications
    // http://www.linuxjournal.com/article/3641    

    Py_SetProgramName(binary_name.data());
    //PyEval_InitThreads();
    Py_InitializeEx(0); // suppress signal handling

    static char* argv[] = {
        binary_name.data(),
        binary_path.data()
    };

    // set python program name and path to installation dir
    // python uses the program name internally to look up paths to python
    // extensions and modules
    PySys_SetArgv(2, argv);

    static PyMethodDef methods[] = {
        {"add_log", newsflash_add_log, METH_VARARGS, "Add a log entry to the application log."},
        {"write_output", newsflash_write_output, METH_VARARGS, "Write to output in the host application."},
        {"system", newsflash_system, METH_VARARGS, "Execute a system command with redirected output."},
        {nullptr, nullptr, 0, nullptr} // sentinel
    };

    Py_InitModule("newsflash", methods);


    //thread_state_ = PyThreadState_Get();
    //interp_state_ = 

}

python::~python()
{}

void python::loadstate(settings& s)
{
    QDir dir(distdir::path("script"));    
    dir.setNameFilters(QStringList("action_*.py"));

    std::vector<std::unique_ptr<script>> vec;

    const auto& scripts = dir.entryList();
    for (int i=0; i<scripts.size(); ++i)
    {
        QString file = scripts[i];
        QString module = file.remove(".py");
        DEBUG(str("Loading python script: _1", file));

        std::unique_ptr<script> s(new script(module));
        if (s->load())
        {
            vec.push_back(std::move(s));
            INFO(str("Loaded script _1", file));
        }
        else
        {

        }
    }

    // sort the scripts to ascending order based on priority value
    // i.e. smaller values (higher priority) first.
    std::sort(std::begin(vec), std::end(vec), 
        [](std::unique_ptr<script>& lhs, std::unique_ptr<script>& rhs) {
            return lhs->priority() < rhs->priority();
        });

}

void python::savestate(settings& s)
{}


bool python::eventFilter(QObject* object, QEvent* event)
{
//    if (object != this)
        return QObject::eventFilter(object, event);


}
} // app

#endif // ENABLE_PYTHON