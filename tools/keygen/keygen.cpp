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

#include <QtNetwork/QNetworkInterface>
#include <QCryptographicHash>
#include <QByteArray>
#include <QString>
#include <QList>
#include "secret.h"

namespace keygen
{

QString generate_fingerprint(const QString& seed)
{
    QString str = QString("seed:%1").arg(seed);

    QByteArray hash = QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex();

    return QString::fromLatin1(hash.constData(), hash.size());
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

    return QString::fromLatin1(hash.constData(), hash.size());
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


