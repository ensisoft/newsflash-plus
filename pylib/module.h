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
#include <functional>
#include <vector>
#include <string>
#include "format.h"

namespace python
{
    // a native python module. Greez to Guido for the lack of void* callback pointer. (awsum)
    template<size_t ModuleID>
    class module
    {
        class thunk 
        {
        public:
            virtual ~thunk() = default;
            virtual bool invoke(PyObject* args) = 0;
        private:
        };
        using thunks  = std::vector<thunk*>;
        using methods = std::vector<PyMethodDef>;

        static thunks& get_thunks() 
        { 
            static thunks t;
            return t;
        }
        static methods& get_methods()
        {
            static methods m;
            return m;
        }

        template<size_t MethodID>
        static PyObject* dispatch(PyObject* self, PyObject* args) noexcept
        {
            auto& thunks = get_thunks();
            auto* thunk  = thunks[MethodID];
            if (!thunk->invoke(args))
                return nullptr;

            Py_INCREF(Py_None);
            return Py_None;
        }

        template<size_t MethodID>
        static void add_method(const char* name, const char* doc, thunk* tptr)
        {
            auto& thunks  = get_thunks();
            auto& methods = get_methods();

            if (MethodID >= thunks.size())
                thunks.resize(MethodID + 1, nullptr);

            assert(thunks[MethodID] == nullptr);
            thunks[MethodID] = tptr;

            PyMethodDef def;
            def.ml_doc   = doc;
            def.ml_flags = METH_VARARGS;
            def.ml_meth  = dispatch<MethodID>;
            def.ml_name  = name;
            methods.push_back(def);
        }

    public:
        template<size_t MethodID>
        static void method(const char* name, std::function<void()> f)
        {
            struct thunkimpl : public thunk
            {
                std::function<void()> func;

                virtual bool invoke(PyObject*) override
                {
                    func();
                    return true;
                }
            };
            static thunkimpl impl;
            impl.func = std::move(f);

            add_method<MethodID>(name, "blalba", &impl);
        }

        template<size_t MethodID, typename A>
        static void method(const char* name, std::function<void (A)> f)
        {
            struct thunkimpl : public thunk
            {
                std::function<void(A)> func;

                virtual bool invoke(PyObject* args) override
                {
                    A value;
                    static auto str = fmt(value);
                    if (!PyArg_ParseTuple(args, str.c_str(), &value))
                        return false;

                    func(value);
                    return true;
                }
            };
            static thunkimpl impl;
            impl.func = std::move(f);

            add_method<MethodID>(name, "blablba", &impl);
        }

        template<size_t MethodID, typename A1, typename A2>
        static void method(const char* name, std::function<void (A1, A2)> f)
        {
            struct thunkimpl : public thunk
            {
                std::function<void(A1, A2)> func;

                virtual bool invoke(PyObject* args) override
                {
                    A1 arg1;
                    A2 arg2;
                    static auto str = fmt(arg1, arg2);
                    if (!PyArg_ParseTuple(args, str.c_str(), &arg1, &arg2))
                        return false;

                    func(arg1, arg2);
                    return true;
                }
            };
            static thunkimpl impl;
            impl.func = std::move(f);

            add_method<MethodID>(name, "blabla", &impl);
        }

        template<size_t MethodID, typename A1, typename A2, typename A3>
        static void method(const char* name, std::function<void(A1, A2, A3)> f)
        {
            struct thunkimpl : public thunk
            {
                std::function<void(A1, A2, A3)> func;

                virtual bool invoke(PyObject* args) override
                {
                    A1 arg1;
                    A2 arg2;
                    A3 arg3;
                    static auto str = fmt(arg1, arg2, arg3);
                    if (!PyArg_ParseTuple(args, str.c_str(), &arg1, &arg2, &arg3))
                        return false;

                    func(arg1, arg2, arg3);
                    return true;
                }
            };
            static thunkimpl impl;
            impl.func = std::move(f);

            add_method<MethodID>(name, "blabal", &impl);
        }


        static void install(std::string name)
        {
            PyMethodDef sentinel {NULL, NULL, 0, NULL};
            auto& methods = get_methods();
            methods.push_back(sentinel);
            auto ret = Py_InitModule(name.c_str(), &methods[0]);
            if (ret == nullptr)
                throw std::runtime_error("module failed to install");
        }
    private:
    };

} // python