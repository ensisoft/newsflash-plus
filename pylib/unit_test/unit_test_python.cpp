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

#include "../python.h"
#include "../script.h"

void unit_test_script()
{
    // set a path to the our working folder from which
    // the modules are expected to be found.
    pylib::python python("/home/enska/coding/newsflash/pylib/unit_test/python");
    
    // try loading a broken script, we should get an exception
    {
        try
        {
            pylib::script script("broken");

            BOOST_REQUIRE(!"exception was excepted");
        }
        catch (const pylib::exception& e)
        {
            std::cout << e.what();
            std::cout << "\n";
            std::cout << e.pyerr();
            std::cout << std::endl;
        }
    }

    // try loading a correct script.
    {
        pylib::script script("correct");

        BOOST_REQUIRE(script.module() == "correct");
        BOOST_REQUIRE(script.docstring() == "this is the docstring.");

        int value = 0;
        BOOST_REQUIRE(script.get_attribute("int_value", value));
        BOOST_REQUIRE(value == 123);

        BOOST_REQUIRE(!script.get_attribute("booofaosg", value));
    }

    // call a function
    {
        pylib::script script("funcs");

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
        catch (const pylib::exception& e)
        {
            std::cout << e.what();
            std::cout << "\n";
            std::cout << e.pyerr();
            std::cout << std::endl;            
        }

        try
        {
            script.call("broken_function");
            BOOST_REQUIRE(!"exception was expected");
        }
        catch (const pylib::exception& e)
        {
            std::cout << e.what();
            std::cout << "\n";
            std::cout << e.pyerr();
            std::cout << std::endl;            
        }
    }


}

void unit_test_module()
{
}

// spawn multiple threads and see what happens in the python
// our python binding (and python lib) should be thread safe
void unit_test_threads()
{

}

int test_main(int argc, char* argv[])
{


    unit_test_script();
    unit_test_module();
    unit_test_threads();

    return 0;
}