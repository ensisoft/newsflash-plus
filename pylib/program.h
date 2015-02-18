// Copyright (c) 2005-2015 Sami Väisänen, Ensisoft 
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

namespace python
{
    // any program that wants to use python should first create an instance
    // of the program object. this will enable the program to later 
    // call into python safely.
    class program 
    {
    public:
        // initialize program/python with the path to the installation
        // location so that it knows where to load the .dll and .py modules.
        // unfortunately the API is based on char* which means that Unicode paths
        // that cannot be represented in extended ASCII will not work as expected.        
        program(std::string path, int argc, char* argv[]) : path_(std::move(path)), state_(nullptr)
        {
            Py_SetProgramName(&path_[0]);

            // this acquires the global interpreter lock. we must 
            // release it later explicitly. (how f* stupid..)
            PyEval_InitThreads();

            // suppress signal handling and create our thread state
            Py_InitializeEx(0);             

            PySys_SetArgv(argc, argv);

            state_ = PyThreadState_Get();

            // release the implicit lock
            PyEval_ReleaseLock();
        }
        program(const program&) = delete;
        program& operator=(const program&) = delete;

       ~program()
        {
            PyEval_AcquireLock();            
            PyThreadState_Swap(state_);            
            Py_Finalize();
        }
    private:
        std::string path_;
    private:
        PyThreadState* state_;
    };
} // python