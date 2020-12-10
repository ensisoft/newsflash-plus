
#include "newsflash/config.h"

#include <stdexcept>
#include <string>

#include "socket.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "sslcontext.h"

namespace {

thread_local std::string error_string;

} // namespace

extern "C"
{

void* MakeNewsflashSocketUnsafe(bool ssl)
{
    try
    {
        if (ssl)
        {
            newsflash::openssl_init();
            return new newsflash::SslSocket();
        }
        return new newsflash::TcpSocket();
    }
    catch (const std::exception& e)
    {
        error_string = e.what();
    }
    return nullptr;
}

const char* GetSocketAllocationFailureString()
{
    return error_string.c_str();
}

} // extern "C"