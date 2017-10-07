#include <QtGui/QApplication>
#include "../smtpclient.h"
#include "../mimetext.h"

#include <iostream>

QString readString(const char* text)
{
    std::cout << text;
    std::string s;
    std::getline(std::cin, s);
    return QString::fromUtf8(s.c_str(), s.size());
}

int readInt(const char* text)
{
    int ret = 0;
    std::cout << text;
    std::cin >> ret;
    return ret;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // This is a first demo application of the SmtpClient for Qt project

    // First we need to create an SmtpClient object
    // We will use the Gmail's smtp server (smtp.gmail.com, port 465, ssl)
    QString server   = readString("email server (e.g. smtp.gmail.com:");
    QString username = readString("username: ");
    QString password = readString("password: ");
    QString recipient = readString("recipient: ");


    SmtpClient smtp;
    smtp.setHost(server);
    smtp.setPort(465);
    smtp.setConnectionType(SmtpClient::SslConnection);
    smtp.setUsername(username);
    smtp.setPassword(password);

    // Now we create a MimeMessage object. This will be the email.

    MimeMessage message;

    message.setSender(new EmailAddress("your_email_address@gmail.com", "Your Name"));
    message.addRecipient(new EmailAddress(recipient, "Recipient's Name"));
    message.setSubject("SmtpClient for Qt - Demo");

    // Now add some text to the email.
    // First we create a MimeText object.

    MimeText text;

    text.setText("Hi,\nThis is a simple email message.\n");

    // Now add it to the mail

    message.addPart(&text);

    // Now we can send the mail

    smtp.connectToHost();
    smtp.loginToServer();
    smtp.sendMail(message);
    smtp.quit();

}