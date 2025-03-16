// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <openssl/engine.h>
#  include <openssl/conf.h>
#  include <openssl/ssl.h>
#  include <openssl/err.h>
#include "newsflash/warnpop.h"

#include <cassert>
#include <stdexcept>

#include "sslcontext.h"
#include "platform.h"

namespace {
    class context;

    std::size_t g_refcount;
    std::mutex  g_mutex;
    context*    g_context;

    class context
    {
    public:
        context() : context_(nullptr)
        {
            newsflash::openssl_init();
        }

       ~context()
        {
            if (context_)
                SSL_CTX_free(context_);
        }

        void init()
        {
            SSL_METHOD* meth = const_cast<SSL_METHOD*>(SSLv23_method());

            context_ = SSL_CTX_new(meth);
            if (!context_)
               throw std::runtime_error("SSL_CTX_new");

            SSL_CTX_set_cipher_list(context_, "ALL");
        }

        SSL_CTX* ctx()
        {
            return context_;
        }

    private:
        SSL_CTX* context_;
    };

} // namespace

namespace newsflash
{

sslcontext::sslcontext()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_context)
    {
        assert(g_refcount == 0);
        // need to store the instance pointer, because
        // when we initialize the SSL it already performs
        // callbacks to the locking functions, which require
        // the locks throught the instance pointer.
        // hence we need the two-phase construction here.
        g_context = new context();
        try
        {
            g_context->init();
        }
        catch (const std::exception&)
        {
            delete g_context;
            g_context = nullptr;
            throw;
        }
    }
    ++g_refcount;
}

sslcontext::~sslcontext()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (--g_refcount == 0)
    {
        delete g_context;
        g_context = nullptr;
    }
}

sslcontext::sslcontext(const sslcontext& other)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    ++g_refcount;
}

SSL_CTX* sslcontext::ssl()
{
    return g_context->ctx();
}

// https://www.openssl.org/support/faq.html#PROG2
// https://www.openssl.org/support/faq.html#PROG13
struct openssl {
    openssl()
    {
        locks = new std::mutex[CRYPTO_num_locks()];

        SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        ERR_load_crypto_strings();

        OpenSSL_add_all_algorithms();

        // these callback functions need to be set last because
        // the setup functions above call these (especially the locking function)
        // which calls back to us and whole thing goes astray.
        // Calling the functions above without locking should be safe
        // considering that this constructor code is already protected from MT access
        // If there are random crashes, maybe this assumption is wrong
        // and locking needs to be taken care of already.
        CRYPTO_set_id_callback(identity_callback);
        CRYPTO_set_locking_callback(lock_callback);
    }

   ~openssl()
    {
        CRYPTO_set_locking_callback(nullptr);

        ERR_free_strings();
        EVP_cleanup();
        CRYPTO_cleanup_all_ex_data();
        ENGINE_cleanup();
        CONF_modules_free();

        delete [] locks;
    }

    static
    unsigned long identity_callback()
    {
        return newsflash::get_thread_identity();
    }

    static
    void lock_callback(int mode, int index, const char*, int)
    {
        if (mode & CRYPTO_LOCK)
            locks[index].lock();
       else locks[index].unlock();
    }

    static std::mutex* locks;
};

std::mutex* openssl::locks;

void openssl_init()
{
    // thread safe per c++11 rules
    static openssl lib;
}

} // newsflash
