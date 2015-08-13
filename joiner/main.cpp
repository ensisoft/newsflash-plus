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

#undef QT_NO_DEBUG_OUTPUT

#include <QtCore/QCoreApplication>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QtGlobal>
#include <QtDebug>
#include <QRegExp>
#include <QFileInfo>
#include <set>
#include <map>

QString joinPath(QString lhs, QString rhs)
{
    QDir dir(lhs + "/" + rhs);
    return dir.absolutePath();
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    bool cleanup   = false;
    bool overwrite = false;
    bool help      = false;
    QString folder;

    const auto& cmds = app.arguments();
    for (int i=1; i<cmds.size(); ++i)
    {
        const auto name = cmds[i];
        if (name == "--folder")
        {
            if (i + 1 < cmds.size())
            {
                folder = cmds[i+1];
            }
        }
        else if (name == "--cleanup")
            cleanup = true;
        else if (name == "--overwrite")
            overwrite = true;
        else if (name == "--help")
            help = true;
    }
    if (help)
    {
        qDebug() << "HJSplit file joiner";
        qDebug() << "--folder\tSpecify the scanning folder.";
        qDebug() << "--cleanup\tRemove HJSplit files after joining.";
        qDebug() << "--overwrite\tOverwrite the target file if it exists.";
        qDebug() << "--help\t\tPrint this help and exit.";
        return 0;
    }


    if (folder.isEmpty())
    {
        qCritical("Missing argument");
        return 1;
    }

    qDebug() << "Scanning folder" << folder;

    std::map<QString, std::set<QString>> parts;

    QRegExp reg("\\.\\d{3}$");


    QDir dir;
    dir.setPath(folder);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    const auto entries = dir.entryList();
    for (const auto& file : entries)
    {
        if (reg.indexIn(file) == -1)
            continue;

        QString key = file;
        key.chop(4);

        auto& set = parts[key];
        set.insert(file);

        qDebug() << "Found:" << file << "->" << key;
    }

    for (auto it = parts.begin(); it != parts.end(); ++it)
    {
        const auto& file  = it->first;
        const auto& parts = it->second;

        QFileInfo info(joinPath(folder, file));
        if (info.exists() && !overwrite)
        {
            qDebug() << file << "already exists. Refusing to overwrite.";
        }
        else
        {
            QFile out(joinPath(folder, file));
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                qCritical() << "Error: Failed to open:" << file;
                return 1;
            }
            for (const auto& part : parts)
            {
                // dunno wth is the .000 file, looks like some sort of descriptor
                if (!part.endsWith(".000"))
                {
                    QFile in(joinPath(folder, part));
                    if (!in.open(QIODevice::ReadOnly))
                    {
                        qCritical() << "Error: Failed to open:" << part;
                        return 1;
                    }
                    QByteArray data = in.readAll();
                    out.write(data);
                    qDebug() << "Joined:" << part << "->" << file;
                }
            }
        }
        if (cleanup)
        {
            for (const auto& part : parts)
            {
                QFile::remove(joinPath(folder, part));
                qDebug() << "Deleted: " << part;
            }
        }
    }

    return 0;
}