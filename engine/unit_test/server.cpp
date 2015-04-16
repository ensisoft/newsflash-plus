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

#include <newsflash/config.h>
#include <functional>
#include <iostream>
#include <fstream>
#include <thread>
#include <sstream>
#include <vector>
#include "../socketapi.h"
#include "../sockets.h"

std::string recv_command(newsflash::native_socket_t sock)
{
    std::string cmd;
    cmd.resize(64);
    const auto bytes = ::recv(sock, &cmd[0], cmd.size(), 0);
    if (bytes < 0)
    {
        if (errno == ECONNRESET)
            return "";
        throw std::runtime_error("socket recv errro");
    }
    else if (bytes == 0)
        return "";
    else if (cmd[bytes-1] != '\n' || cmd[bytes-2] != '\r')
        throw std::runtime_error("incomplete command");

    cmd.resize(bytes - 2);
    std::cout << "> " << cmd << std::endl;

    return cmd;
}

void send_response(newsflash::native_socket_t sock, std::string resp, bool print = true)
{
    resp.append("\r\n");
    int sent = 0;
    do 
    {
        const char* ptr = resp.data() + sent;
        const auto len  = resp.size() - sent;
        const auto bytes = ::send(sock, ptr, len, 0);
        if (bytes < 0)
            throw std::runtime_error("socket send error");
        sent += bytes;
    } while(sent != resp.size());

    if (print)
    {
        resp.resize(resp.size() - 2);
        std::cout << "< " << resp << std::endl;
    }
}

void service_client(newsflash::native_socket_t sock)
{
    bool is_authenticated = false;

    send_response(sock, "200 welcome");
    for (;;)
    {
        const auto cmd = recv_command(sock);
        if (cmd.empty())
        {
            std::cout << "Client closed socket...\n";
            break;
        }

        if (cmd == "QUIT") {
            newsflash::closesocket(sock);
            break;
        }
        else if (cmd == "CAPABILITIES")
        {
            send_response(sock, "500 what?");
        }
        else if (cmd == "MODE READER")
        {
            if (is_authenticated)
                send_response(sock, "200 posting allowed");
            else
            {
                send_response(sock, "480 authentication required");
            }
        }
        else if (cmd == "AUTHINFO USER pass")
        {
            send_response(sock, "381 password required");
        }
        else if (cmd == "AUTHINFO PASS pass")
        {
            send_response(sock, "281 authentication accepted");
            is_authenticated = true;
        }
        else if (cmd == "AUTHINFO USER fail")
        {
            send_response(sock, "482 authentication rejected");
        }
        else if (cmd == "AUTHINFO PASS fail")
        {
            send_response(sock, "482 authentication rejected");
        }

        else if (cmd == "BODY 1")
        {
            send_response(sock, "420 no article with that message id");
        }
        else if (cmd == "BODY 2")
        {
            send_response(sock, "222 body follows");
            std::ifstream in("test_data/newsflash_2_0_0.yenc", std::ios::binary);
            if (!in.is_open())
                throw std::runtime_error("failed to open test_data/newsflash_2_0_0.yenc");

            std::string line;
            while (std::getline(in, line))
            {
                if (line.empty())
                    throw std::runtime_error("empty line!");
                if (line[0] == '.')
                    line = "." + line;
                send_response(sock, line, false);
            }
            send_response(sock, ".", false);
        }
        else if (cmd == "BODY 3")
        {
            send_response(sock, "222 body follows");
            std::ifstream in("test_data/newsflash_2_0_0.uuencode");
            if (!in.is_open())
                throw std::runtime_error("failed to open test_data/newsflash_2_0_0.uuencode");

            std::string line;
            while (std::getline(in, line))
            {
                if (line.empty())
                    throw std::runtime_error("empty line");
                if (line[0] == '.')
                    line = '.' + line;
                send_response(sock, line, false);
            }
            send_response(sock, ".", false);
        }
        else if (cmd == "BODY 4")
        {
            send_response(sock, "222 body follows");
            std::string content = 
               "here is some textual content\n"
               "first line.\n"
               ". second line starts with a dot\n"
               ".. two dots\n"
               "foo . bar\n"
               "...\n"
               "end\n";

            std::stringstream ss(content);

            std::string line;
            while (std::getline(ss, line))
            {
                if (line[0] == '.')
                    line = "." + line;
                send_response(sock, line, false);
            }
            send_response(sock, ".");
        }
        else if (cmd == "GROUP alt.binaries.foo")
        {
            send_response(sock, "211 5 1 5 alt.binaries.foo Group selected");
        }
        else if (cmd == "GROUP alt.binaries.bar")
        {
            send_response(sock, "411 no such newsgroup");
        }
        else if (cmd == "GROUP alt.binaries.authenticate")
        {

            send_response(sock, "480 authentication required");
        }
        else
        {
            send_response(sock, "500 what?");
        }

    }
    std::cout << "Thread exiting...\n";
}

int main(int argc, char*[])
{
    auto port = 1919;
    auto sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    for (;;)
    {
        struct sockaddr_in addr {0};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        const int ret = ::bind(sock, static_cast<sockaddr*>((void*)&addr), sizeof(addr));
        if (ret != newsflash::OS_SOCKET_ERROR)
            break;

        std::cout << "Bind failed. Retrying...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    ::listen(sock, 10);

    std::cout << "Server listening on " << port << "\n";

    for (;;)
    {
        struct sockaddr_in addr = {0};
        socklen_t len = sizeof(addr);
        auto client = ::accept(sock, static_cast<sockaddr*>((void*)&addr), &len);

        std::cout << "Got new client connection. Threading...\n";

//        std::thread thread(std::bind(service_client, client));
//        thread.detach();
        service_client(client);
    }
    return 0;
}
