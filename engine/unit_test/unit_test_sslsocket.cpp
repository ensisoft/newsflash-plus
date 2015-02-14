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

#include <boost/test/minimal.hpp>
#include <condition_variable>
#include <openssl/err.h>
#include <functional>
#include <thread>
#include <chrono>
#include "../socketapi.h"
#include "../sslsocket.h"
#include "../sslcontext.h"
#include "../platform.h"
#include "../types.h"
#include "unit_test_common.h"

#if defined (LINUX_OS)
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <arpa/inet.h>
#  include <netinet/in.h>
#endif

using namespace newsflash;


void test_connection_failure()
{
    // refused
    {
        uint32_t addr;
        resolve_host_ipv4("127.0.0.1", addr);

        sslsocket sock;
        sock.begin_connect(addr, 8000);

        newsflash::wait(sock);
        REQUIRE_EXCEPTION(sock.complete_connect());
    }
}

struct checkpoint {
    std::mutex m;
    std::condition_variable c;
    bool hit;

    checkpoint() : hit(false)
    {}

    void check() 
    {
        std::unique_lock<std::mutex> lock(m);
        while (!hit)
            c.wait(lock);
    }

    void mark()
    {
        std::lock_guard<std::mutex> lock(m);
        hit = true;
        c.notify_one();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m);
        hit = false;        
    }
};

struct buffer {
    int   len;
    char* data;
    char* buff;
} buffers[] = {
    {10},
    {1024},
    {1024 * 1024},
    {1024 * 1024 * 5},
    {555}
};

checkpoint open_socket;
checkpoint send_buffer;

// DH ciphers are the only ciphers that do not require a certificate.
// for DH ciphers the DH key parameters must be set.

void ssl_server_main(int port)
{
    auto listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    while (::bind(listener, static_cast<sockaddr*>((void*)&addr), sizeof(addr)) == OS_SOCKET_ERROR)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    BOOST_REQUIRE(listen(listener, 1) != OS_SOCKET_ERROR);

    open_socket.mark();

    // get a client connection
    memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    auto fd = ::accept(listener, static_cast<sockaddr*>((void*)&addr), &len);
    BOOST_REQUIRE(fd != OS_INVALID_SOCKET);


    // create new SSL server context.
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
    BOOST_REQUIRE(ctx);

    // create file bio...
    BIO* pem = BIO_new_file("test_data/dh1024.pem", "r");
    BOOST_REQUIRE(pem);

    // and read the DH key parameters from the file bio
    DH* dh = PEM_read_bio_DHparams(pem, NULL, NULL, NULL);
    BOOST_REQUIRE(dh);


    // set the DH key parameters for the session key generation.
    // the actual key is generated on the fly and the parameters
    // coming from the dh file can be reused. 
    BOOST_REQUIRE(SSL_CTX_set_tmp_dh(ctx, dh) == 1);
    //BOOST_REQUIRE(SSL_CTX_set_tmp_rsa(ctx, rsa) == 1);
    BOOST_REQUIRE(SSL_CTX_set_cipher_list(ctx, "ALL") == 1);

    SSL* ssl = SSL_new(ctx);
    BIO* bio = BIO_new_socket(fd, BIO_NOCLOSE);
    SSL_set_bio(ssl, bio, bio);

    BOOST_REQUIRE(SSL_accept(ssl) == 1);

    sslsocket sock(fd, get_wait_handle(fd), ssl, bio);


    // receive data
    for (auto& buff : buffers)
    {
        std::memset(buff.buff, 0, buff.len);
        int recv = 0;
        do
        {
            auto can_read = sock.wait(true, false);
            newsflash::wait(can_read);
            int ret = sock.recvsome(buff.buff + recv, buff.len - recv);
            recv += ret;
        }
        while (recv != buff.len);

        BOOST_REQUIRE(!std::memcmp(buff.data, buff.buff, buff.len));

        send_buffer.mark();
    }

    sock.close();

    BIO_free(pem);
    SSL_CTX_free(ctx);

    newsflash::closesocket(listener);
}

void test_connection_success()
{
    for (auto& buff : buffers)
    {
        buff.data = new char[buff.len];
        buff.buff = new char[buff.len];
        fill_random(buff.data, buff.len);
    }

    // open ssl server
    std::thread server(std::bind(ssl_server_main, 8001));

    open_socket.check();

    std::uint32_t addr;
    resolve_host_ipv4("127.0.0.1", addr);

    // connect the client socket
    sslsocket sock;
    sock.begin_connect(addr, 8001);
    newsflash::wait(sock);
    sock.complete_connect();

    // transfer data
    for (auto& buff : buffers)
    {
        send_buffer.clear();

        int sent = 0;
        do
        {
            auto can_write = sock.wait(false, true);
            newsflash::wait(can_write);
            int ret = sock.sendsome(buff.data + sent, buff.len - sent);
            sent += ret;
        }
        while (sent != buff.len);

        send_buffer.check();
    }

    server.join();

    // cleanup
    for (auto& buff : buffers)
    {
        delete [] buff.data;
        delete [] buff.buff;
    }
 
}

int test_main(int argc, char* argv[])
{
    // $ netstat --tcp --numeric --listen --program

    newsflash::openssl_init();

    test_connection_failure();
    test_connection_success();

    return 0;
}
