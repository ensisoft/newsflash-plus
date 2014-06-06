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

#include <QtNetwork/QNetworkInterface>
#include <QCryptographicHash>
#include <QByteArray>
#include <QString>
#include <QList>

namespace {
    const char* const FALLBACK = "00-14-78-F6-01-71";
    const char* const BACKDOOR = "727f1c60d2c6b9ad8fb9783195b5d5db";
} // namespace

namespace keygen
{

QString generate_fingerprint(const QString& seed)
{
    QString str = QString("seed:%1").arg(seed);

    QByteArray hash = QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex();

    return QString::fromAscii(hash.constData(), hash.size());
}

QString generate_fingerprint()
{
    QList<QNetworkInterface> nics = QNetworkInterface::allInterfaces();

    const QNetworkInterface* nic = NULL;
    for (int i=0; i<nics.size(); ++i)
    {
        const QNetworkInterface& n = nics[i];
        if (!(n.flags() & QNetworkInterface::IsLoopBack))
        {
            nic = &n;
            break;
        }
    }
    if (!nic)
        return generate_fingerprint(FALLBACK);
        
    return generate_fingerprint(nic->hardwareAddress());
}

QString generate_keycode(const QString& fingerprint)
{
    QString str = QString("keycode:%1").arg(fingerprint);
    
    QByteArray hash = QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex();

    return QString::fromAscii(hash.constData(), hash.size());
}

bool verify_code(const QString& keycode)
{
    const QList<QNetworkInterface>& nics = QNetworkInterface::allInterfaces();
    QList<QString> macs;
    for (int i=0; i<nics.size(); ++i)
    {
        const QNetworkInterface& nic = nics[i];
        if (!(nic.flags() & QNetworkInterface::IsLoopBack))
            macs.append(nic.hardwareAddress());
    }
    if (macs.isEmpty())
        macs.append(FALLBACK);
    
    for (int i=0; i<macs.size(); ++i)
    {
        QString fingerprint = generate_fingerprint(macs[i]);
        if (generate_keycode(fingerprint) == keycode)
            return true;
    }
    // this is our backdoor for cases where the code is buggy for whatever reason
    // and it won't work for people who have registered.
    // this is not optimal but better than disappointing people
    if (keycode == BACKDOOR)
        return true;

    return false;
}

} // keygen


