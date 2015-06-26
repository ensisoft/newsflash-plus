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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <boost/thread/tss.hpp>
#include <newsflash/warnpop.h>
#include <chrono>

#include <iomanip>
#include "platform.h"
#include "logging.h"

namespace {
    struct TLS {
        newsflash::logger* logger;
    };

    boost::thread_specific_ptr<TLS> threadLogger;
} // namespace

namespace newsflash
{
namespace detail {

    void beg_log_event(std::ostream& out, logevent type, const char* file, int line)
    {
        using namespace std;

        const auto& timeval = get_localtime();

        out << setw(2) << setfill('0') << timeval.hours   << ":"
            << setw(2) << setfill('0') << timeval.minutes << ":"
            << setw(2) << setfill('0') << timeval.seconds << ":"
            << setw(3) << setfill('0') << timeval.millis  << " ";
        out << (char)type << " ";
    }

    void end_log_event(std::ostream& current_stream)
    {
        current_stream << std::endl;
    }

} // detail

logger* get_thread_log()
{
    if (!threadLogger.get())
        return nullptr;

    return threadLogger->logger;
}

logger* set_thread_log(logger* log)
{
    logger* current = nullptr;

    if (!threadLogger.get())
        threadLogger.reset(new TLS);
    else current = threadLogger->logger;

    threadLogger->logger = log;
    return current;
}

void flush_log()
{
    auto* log = get_thread_log();
    if (!log) return;

    log->flush();
}

} // newsflash
