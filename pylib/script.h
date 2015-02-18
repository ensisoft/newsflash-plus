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

#pragma once

#include <Python.h>

#include <stdexcept>
#include <string>
#include <sstream>
#include <cassert>
#include "gil.h"
#include "object.h"
#include "exception.h"
#include "format.h"

namespace python
{
    // a python script.
    class script
    {
    public:
        // load the python script (known as module)
        // the module name should be the name of the script
        // without the .py extension.
        // precondition: module is not an empty string.
        script(const std::string& module)
        {
            assert(!module.empty());
            assert(module.find(".py") == std::string::npos);

            detail::GIL lock;
            detail::object modname(module);
            detail::object modhandle = PyImport_Import(modname.get());
            if (PyErr_Occurred())
                throw python::exception(get_python_error());

            module_ = module;
            handle_ = modhandle;
        }

       ~script()
        {}

        std::string docstring() // const
        {
            std::string str;
            get_attribute("__doc__", str);
            return str;
        }

        std::string module() const
        {
            return module_;
        }

        bool has_attribute(const char* name) // const
        {
            detail::GIL lock;
            const detail::object attr = PyObject_GetAttrString(handle_.get(), name);
            PyErr_Clear();
            return attr;
        }

        bool has_function(const char* name) // const
        {
            detail::GIL lock;
            detail::object attr = PyObject_GetAttrString(handle_.get(), name);
            PyErr_Clear();
            if (attr && PyCallable_Check(attr.get()))
                return true;
            return false;
        }

        template<typename T>
        bool get_attribute(const char* name, T& val) // const
        {
            detail::GIL lock;
            const detail::object attr = PyObject_GetAttrString(handle_.get(), name);
            PyErr_Clear();
            if (!attr)
                return false;
            return attr.value(val);
        }

        // thanks again Guido for not providing a va_list version of CallFunction
        // so we have to type out all the functions here instead of just type shimming
        // with templates.... you're my herO!
        void call(const char* function)
        {
            detail::GIL lock;
            detail::object func = PyObject_GetAttrString(handle_.get(), function);
            if (PyErr_Occurred())
                throw python::exception(std::string("no such function: ") + function);

            detail::object ret = PyObject_CallFunction(func.get(), nullptr);
            if (PyErr_Occurred())
                throw python::exception(get_python_error());
        }

        template<typename A1>
        void call(const char* function, A1 arg)
        {
            detail::GIL lock;
            detail::object func = PyObject_GetAttrString(handle_.get(), function);
            if (PyErr_Occurred())
                throw python::exception(std::string("no such function: ") + function);

            static auto str = python::fmt(arg);

            detail::object ret = PyObject_CallFunction(func.get(), &str[0], arg);
            if (PyErr_Occurred())
                throw python::exception(get_python_error());
        }

        template<typename A1, typename A2>
        void call(const char* function, A1 arg1, A2 arg2)
        {
            detail::GIL lock;
            detail::object func = PyObject_GetAttrString(handle_.get(), function);
            if (PyErr_Occurred())
                throw python::exception(std::string("no such function: ") + function);

            static auto str = python::fmt(arg1, arg2);

            detail::object ret = PyObject_CallFunction(func.get(), &str[0], arg1, arg2);
            if (PyErr_Occurred())
                throw python::exception(get_python_error());
        }        

        template<typename A1, typename A2, typename A3>
        void call(const char* function, A1 arg1, A2 arg2, A3 arg3) 
        {
            detail::GIL lock;
            detail::object func = PyObject_GetAttrString(handle_.get(), function);
            if (PyErr_Occurred())
                throw python::exception(std::string("no such function: ") + function);

            static auto str = python::fmt(arg1, arg2, arg3);

            detail::object ret = PyObject_CallFunction(func.get(), &str[0], arg1, arg2, arg3);
            if (PyErr_Occurred())
                throw python::exception(get_python_error());
        }        

    private:
        std::string module_;
    private:
        detail::object handle_;
    };

} // python
