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


#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <cerrno>
#include <cstdlib>

pid_t child;

void kill_handler(int)
{
    //std::cout << "signal handler" << std::endl;
    // the host application will "cancel" an external process by sending a kill signal to it
    // however on linux the kill signal will cause the process to terminate with 0 return code
    // which implies there was no error. We have no means of specifying an alternative return value
    // for the process except to wrap it up.
    // so we got kill signal...
    kill(child, SIGKILL);

    
    //http://stackoverflow.com/questions/1101957/are-there-any-standard-exit-status-codes-in-linux
    // i'm not sure if the exit code for KILL signal is a kernel feature or a shell feature
    // but lets use this exit code anyway
    const int SIGKILL_EXIT = 137;

    exit(SIGKILL_EXIT);
}

int main(int argc, char* argv[])
{
    signal(SIGTERM, kill_handler);
    signal(SIGINT,  kill_handler);

    child = ::fork();
    if (child == -1)
    {
        std::cerr << "cannot fork child";
        return 1;
    } 
    else if (child > 0)
    {
        int status = 0;
        waitpid(child, &status, 0);
        return WEXITSTATUS(status);
    }
    
    // create NULL terminated argument array
    std::vector<char*> args;
    for (int i=1; i<argc; ++i)
        args.push_back(argv[i]);
    
    args.push_back(NULL);

    // if this function returns an error has occurred.
    // execvp searches the path for the binary just like the shell
    execvp(argv[1], &args[0]);

    std::cerr << strerror(errno);
    return 1;
}
