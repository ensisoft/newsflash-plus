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

#include <boost/test/minimal.hpp>
#include <thread>
#include "../program.h"
#include "../script.h"
#include "../module.h"

void unit_test_script()
{
    // try loading a broken script, we should get an exception
    {
        try
        {
            python::script script("broken");

            BOOST_REQUIRE(!"exception was excepted");
        }
        catch (const python::exception& e)
        {
            std::cout << e.what();
            std::cout << std::endl;
        }
    }

    // try loading a correct script.
    {
        python::script script("correct");

        BOOST_REQUIRE(script.module() == "correct");
        BOOST_REQUIRE(script.docstring() == "this is the docstring.");

        int value = 0;
        BOOST_REQUIRE(script.get_attribute("int_value", value));
        BOOST_REQUIRE(value == 123);

        BOOST_REQUIRE(!script.get_attribute("booofaosg", value));
    }

    // call a function
    {
        python::script script("funcs");

        BOOST_REQUIRE(script.has_attribute("nonfunction"));
        BOOST_REQUIRE(script.has_function("nonfunction") == false);
        BOOST_REQUIRE(script.has_function("test_arg_int"));

        script.call("test");
        script.call("test_arg_int", 1234);
        script.call("test_arg_str", "ARDVARK");
        script.call("test_arg_str_int", 1234, "ARDVARK");

        try
        {
            script.call("raise_exception");
            BOOST_REQUIRE(!"exception was expected");
        }
        catch (const python::exception& e)
        {
            std::cout << e.what();
            std::cout << std::endl;            
        }

        try
        {
            script.call("broken_function");
            BOOST_REQUIRE(!"exception was expected");
        }
        catch (const python::exception& e)
        {
            std::cout << e.what();
            std::cout << std::endl;            
        }
    }


}

void unit_test_module()
{
    using m = python::module<1>;

    bool void_method_visited = false;
    m::method<0>("test_method_void", [&]() 
    {
        void_method_visited = true;
    });

    bool int_method_visited = false;
    m::method<1, int>("test_method_int", [&](int value) 
    {
       int_method_visited = true;
       BOOST_REQUIRE(value == 123);
    });

    m::install("unit_test");

    python::script script("callback");

    script.call("call_void");
    BOOST_REQUIRE(void_method_visited);

    script.call("call_int");
    BOOST_REQUIRE(int_method_visited);

}

void thread_main(int identity)
{
    python::script script("funcs");

    for (int i=0; i<10000; ++i)
    {
        script.call("test_thread_identity", identity);
    }
}

// spawn multiple threads and see what happens in the python
// our python binding (and python lib) should be thread safe
void unit_test_threads()
{
    std::thread t1(std::bind(thread_main, 1));
    std::thread t2(std::bind(thread_main, 2));

    t1.join();
    t2.join();
}

int test_main(int argc, char* argv[])
{
    // set a path to the our working folder from which
    // the modules are expected to be found.
    python::program python("/home/enska/coding/newsflash/pylib/unit_test/python",
        argc, argv);

    unit_test_script();
    unit_test_module();
    //unit_test_threads();
    return 0;
}