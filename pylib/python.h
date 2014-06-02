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

namespace pylib
{
    // main python interface class. any application
    // that wants to use python should first create an instance
    // of this class and use it to configure the global 
    // python state. 
    class python
    {
    public:
        // initialize python with the absolute path to the 
        // running process's binary on the file system.
        // python uses this information to look for modules
        // (both native and python)
        python(const std::string& binpath)
        {
            // We have to tell python the installation location
            // so that it knows where to look for modules (both .dll and .py).
            // unfortunately the API is based on char* which means that Unicode paths
            // that cannot be represented in extended ASCII will not work as expected.

            // initialize python
            // Embedding Python in Multi-Threaded C/C++ applications
            // http://www.linuxjournal.com/article/3641

            // this is non-const...
            Py_SetProgramName(&path_[0]);

            // this acquires the global interpreter lock. we must 
            // release it later explicitly. (how f* stupid..)
            PyEval_InitThreads();

            // suppress signal handling and create our thread state
            Py_InitializeEx(0); 

            state_ = PyThreadState_Get();

            assert(state_);

            // finally release the lock (see comments above)
            // so that we're good to go
            PyEval_ReleaseLock();            
        }

       ~python()
        {
            PyEval_AcquireLock();
            PyThreadState_Swap(state_);
            Py_Finalize();            
        }
    private:
        std::string path_;

        PyThreadState* state_;
    };

} // pylib
