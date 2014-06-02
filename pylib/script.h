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

#include <string>
#include <cassert>
#include "gil.h"
#include "object.h"
#include "exception.h"

namespace pylib
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

            GIL lock;
            object modname(module);
            object modhandle = PyImport_Import(modname.get());
            if (PyErr_Occurred())
            {
                throw pylib::exception("failed to import module",
                    get_python_error());
            }
            module_ = module;
            handle_ = modhandle;
        }

       ~script()
        {}

        std::string docstring() const
        {
            std::string str;
            get_attribute("__doc__", str);
            return str;
        }

        std::string module() const
        {
            return module_;
        }

        bool has_attribute(const char* name) const
        {
            GIL lock;
            const object attr = PyObject_GetAttrString(handle_.get(), name);
            PyErr_Clear();
            return attr;
        }

        bool has_function(const char* name) const
        {
            GIL lock;
            object attr = PyObject_GetAttrString(handle_.get(), name);
            PyErr_Clear();
            if (attr && PyCallable_Check(attr.get()))
                return true;
            return false;
        }

        template<typename T>
        bool get_attribute(const char* name, T& val) const
        {
            GIL lock;
            const object attr = PyObject_GetAttrString(handle_.get(), name);
            PyErr_Clear();
            if (!attr)
                return false;
            return attr.value(val);
        }

        void call(const char* function)
        {
            GIL lock;
            object func = PyObject_GetAttrString(handle_.get(), function);
            if (PyErr_Occurred())
            {
                throw pylib::exception("failed to find function",
                    get_python_error());
            }
            object ret = PyObject_CallFunctionObjArgs(func.get(), nullptr);
            if (PyErr_Occurred())
            {
                throw pylib::exception("failed to call function",
                    get_python_error());
            }
        }

        template<typename T>
        void call(const char* function, const T& arg)
        {
            GIL lock;
            object func = PyObject_GetAttrString(handle_.get(), function);
            if (PyErr_Occurred())
            {
                throw pylib::exception("failed to find function",
                    get_python_error());
            }
            object arg1(arg);
            object ret = PyObject_CallFunctionObjArgs(func.get(), arg1.get(), nullptr);
            if (PyErr_Occurred())
            {
                throw pylib::exception("failed to call function",
                    get_python_error());
            }
        }

        template<typename T, typename F>
        void call(const char* function, const T& arg1, const F& arg2)
        {
            GIL lock;
            object func = PyObject_GetAttrString(handle_.get(), function);
            if (PyErr_Occurred())
            {
                throw pylib::exception("failed to find function",
                    get_python_error());
            }
            object a1(arg1);
            object a2(arg2);
            object ret = PyObject_CallFunctionObjArgs(func.get(), a1.get(), a2.get(), nullptr);
            if (PyErr_Occurred())
            {
                throw pylib::exception("failed to call function",
                    get_python_error());
            }
        }        

        template<typename T, typename F, typename Z>
        void call(const char* function, const T& arg1, const F& arg2, const Z& arg3)
        {
            GIL lock;
            object func = PyObject_GetAttrString(handle_.get(), function);
            if (PyErr_Occurred())
            {
                throw pylib::exception("failed to find function",
                    get_python_error());
            }
            object a1(arg1);
            object a2(arg2);
            object a3(arg3);
            object ret = PyObject_CallFunctionObjArgs(func.get(), a1.get(), a2.get(), a3.get(), nullptr);
            if (PyErr_Occurred())
            {
                throw pylib::exception("failed to call function",
                    get_python_error());
            }
        }        


    private:
        std::string module_;
        mutable object handle_;
    };

} // pylib
