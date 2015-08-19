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
#include <QFile>
#include <QTextStream>
#include <cstdio>
#include <set>
#include <map>


QString joinPath(QString lhs, QString rhs)
{
    QDir dir(lhs + "/" + rhs);
    return dir.absolutePath();
}

void printHelp(QTextStream& out)
{
    out << "HJSplit file joiner\n\n";
    out << "--folder\tSpecify the scanning folder.\n";
    out << "--cleanup\tRemove HJSplit files after joining.\n";
    out << "--overwrite\tOverwrite the target file if it exists.\n";
    out << "--help\t\tPrint this help and exit.\n";
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
    QFile io;
    io.open(stdout, QIODevice::WriteOnly); 

    QTextStream cout(&io);

    if (help)
    {
        printHelp(cout);
        return 0;
    }


    if (folder.isEmpty())
    {
        printHelp(cout);
        return 1;
    }

    cout << "Scanning folder" << folder << "....\n";

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

        cout << "Found:" << file << "->" << key << "\n";
    }

    for (auto it = parts.begin(); it != parts.end(); ++it)
    {
        const auto& file  = it->first;
        const auto& parts = it->second;

        QFileInfo info(joinPath(folder, file));
        if (info.exists() && !overwrite)
        {
            cout << file << "already exists. Refusing to overwrite." << "\n";
        }
        else
        {
            QFile out(joinPath(folder, file));
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                cout << "Error: Failed to open:" << file;
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
                        cout << "Error: Failed to open:" << part << "\n";
                        return 1;
                    }
                    QByteArray data = in.readAll();
                    out.write(data);
                    cout << "Joined:" << part << "->" << file << "\n";
                }
            }
        }
        if (cleanup)
        {
            for (const auto& part : parts)
            {
                QFile::remove(joinPath(folder, part));
                cout << "Deleted: " << part << "\n";
            }
        }
    }

    return 0;
}