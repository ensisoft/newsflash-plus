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
#include <algorithm> // for swap
#include <string>

namespace python
{
    namespace detail {

    class object
    {
    public:
        object() : obj_(nullptr)
        {}

        object(PyObject* obj) : obj_(obj)
        {
            Py_XINCREF(obj_);
        }

        object(const object& other) : obj_(other.obj_)
        {
            Py_XINCREF(obj_);
        }

        explicit 
        object(const char* str) : object(PyString_FromString(str))
        {
            Py_XINCREF(obj_);
        }

        explicit
        object(const std::string& str) : object(str.c_str())
        {}

        explicit
        object(const wchar_t* str) : obj_(PyUnicode_FromUnicode(const_cast<wchar_t*>(str), std::wcslen(str)))
        {
            Py_XINCREF(obj_);
        }

        explicit
        object(const std::wstring& str) : object(str.c_str())
        {}

        explicit 
        object(int i) : obj_(PyInt_FromLong(i))
        {
            Py_XINCREF(obj_);
        }

        explicit 
        object(double d) : obj_(PyFloat_FromDouble(d))
        {
            Py_XINCREF(obj_);
        }

       ~object()
        {
            Py_XDECREF(obj_);
        }

        operator bool() const
        {
            return obj_ != nullptr;
        }

        PyObject* get() 
        {
            return obj_;
        }

        const PyObject* get() const
        {
            return obj_;
        }


        object& operator=(const object& other)
        {
            object tmp(other);
            std::swap(tmp.obj_, this->obj_);
            return *this;
        }        

        bool value(std::string& str) const
        {
            if (obj_ && PyString_Check(obj_))
            {
                str = PyString_AsString(obj_);
                return true;
            }
            return false;            
        }

        bool value(int& val) const
        {
            if (obj_ && PyInt_Check(obj_))
            {
                val = PyInt_AsLong(obj_);
                return true;
            }
            return false;            
        }

    private:
        PyObject* obj_;
    };

    } // detail

} // python