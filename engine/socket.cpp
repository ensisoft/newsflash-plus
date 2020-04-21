
#include "socket.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "sslcontext.h"

extern "C"
{

std::unique_ptr<newsflash::Socket> MakeNewsflashSocket(bool ssl)
{
    if (ssl)
    {
        newsflash::openssl_init();
        return std::make_unique<newsflash::SslSocket>();
    }
    return std::make_unique<newsflash::TcpSocket>();
}

} // extern "C"