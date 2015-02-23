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

#include "debug.h"

namespace debug
{

std::mutex& getLock()
{
    static std::mutex m;
    return m;
}

#define CASE(x) case x: return #x;

QString toString(QProcess::ExitStatus status)
{
    switch (status)
    {
        CASE(QProcess::CrashExit);
        CASE(QProcess::NormalExit);
    }
    Q_ASSERT(0);
    return "";
}

QString toString(QProcess::ProcessError err)
{
    switch (err)
    {
        CASE(QProcess::FailedToStart);
        CASE(QProcess::Crashed);
        CASE(QProcess::Timedout);
        CASE(QProcess::WriteError);
        CASE(QProcess::ReadError);
        CASE(QProcess::UnknownError);
    }
    Q_ASSERT(0);
    return "";    
}

QString toString(QNetworkReply::NetworkError error)
{
    switch (error)
    {
        CASE(QNetworkReply::ConnectionRefusedError);
        CASE(QNetworkReply::RemoteHostClosedError);
        CASE(QNetworkReply::HostNotFoundError);
        CASE(QNetworkReply::TimeoutError);
        CASE(QNetworkReply::OperationCanceledError);
        CASE(QNetworkReply::SslHandshakeFailedError);
        CASE(QNetworkReply::TemporaryNetworkFailureError);
        CASE(QNetworkReply::ProxyConnectionRefusedError);
        CASE(QNetworkReply::ProxyConnectionClosedError);
        CASE(QNetworkReply::ProxyNotFoundError);
        CASE(QNetworkReply::ProxyAuthenticationRequiredError);
        CASE(QNetworkReply::ProxyTimeoutError);
        CASE(QNetworkReply::ContentOperationNotPermittedError);
        CASE(QNetworkReply::ContentNotFoundError);
        CASE(QNetworkReply::ContentReSendError);
        CASE(QNetworkReply::AuthenticationRequiredError);
        CASE(QNetworkReply::ProtocolUnknownError);
        CASE(QNetworkReply::ProtocolInvalidOperationError);
        CASE(QNetworkReply::UnknownNetworkError);
        CASE(QNetworkReply::UnknownProxyError);
        CASE(QNetworkReply::UnknownContentError);
        CASE(QNetworkReply::ContentAccessDenied);
        CASE(QNetworkReply::ProtocolFailure);
        CASE(QNetworkReply::NoError);
    }
    Q_ASSERT(0);
    return "";
}

QString toString(QFile::FileError error)
{
    switch (error)
    {
        CASE(QFile::NoError);
        CASE(QFile::ReadError);
        CASE(QFile::WriteError);
        CASE(QFile::FatalError);
        CASE(QFile::ResourceError);
        CASE(QFile::OpenError);
        CASE(QFile::AbortError);
        CASE(QFile::TimeOutError);
        CASE(QFile::UnspecifiedError);
        CASE(QFile::RemoveError);
        CASE(QFile::RenameError);
        CASE(QFile::PositionError);
        CASE(QFile::ResizeError);
        CASE(QFile::PermissionsError);
        CASE(QFile::CopyError);
    }
    Q_ASSERT(0);
    return "";
}

QString toString(const QNetworkReply& reply)
{
    const auto& u = reply.url();
    const auto& e = reply.error();
    return QString("[url: %1 %2]").arg(toString(u)).arg(toString(e));
}

#undef CASE


} // debug