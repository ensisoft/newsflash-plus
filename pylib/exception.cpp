// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#include <Python.h>

#include <cassert>
#include "exception.h"
#include "object.h"

namespace python
{

std::string get_python_error()
{
    // this is ugly hack. in order to get simple stacktrace/etc
    // error information out of python the simplest way is to
    // *sigh* call back into a python to execute a simple
    // python snippet that gets the stuff done easily.
    // but since calling into python when there's an uncleared error
    // is an error itself... 

    // static const char* formatter = 
    //    "import traceback\n"
    //    "def format_error(etype, value, tb=None):\n"
    //    "\treturn ''.join(traceback.format_exception(etype, value, tb, None))\n"
    //    ;    

    PyObject* type  = nullptr;
    PyObject* value = nullptr;
    PyObject* trace = nullptr;
    PyErr_Fetch(&type, &value, &trace);
    PyErr_Clear(); // clear the exception before calling python again

    detail::object ref_type(type);
    detail::object ref_value(value);
    detail::object ref_trace(trace);

    std::string str = "no error information available";

    detail::object name("pylib");
    detail::object module = PyImport_Import(name.get());
    if (!module)
    {
        PyErr_Clear();
        return str;
    }

    detail::object func = PyObject_GetAttrString(module.get(), "format_error");
    if (!func || !PyCallable_Check(func.get()))
    {
        PyErr_Clear();
        return str;
    }

    detail::object ret = PyObject_CallFunctionObjArgs(func.get(), 
        type, value, trace, nullptr);
    if (ret && PyString_Check(ret.get()))
        ret.value(str);

    PyErr_Clear();
    return str;
}

} // python