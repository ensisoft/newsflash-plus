#include <QtCore/QCoreApplication>
#include <QByteArray>
#include <QString>
#include "keygen.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (argc != 2)
    {
        std::cout << "needs fingerprint parameter!\n\n";
        return 1;
    }
    using namespace keygen;
    
    const QString& fingerprint = QString::fromAscii(argv[1]);
    const QString& keycode     = generate_keycode(fingerprint);
    
    const QByteArray& bytes = keycode.toAscii();
    std::cout.write(bytes.constData(), bytes.size());
    std::cout << std::endl;
    return 0;
}
