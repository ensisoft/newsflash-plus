/*
  Copyright (c) 2011-2012 - Tőkés Attila

  This file is part of SmtpClient for Qt.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  See the LICENSE file for more details.
*/

#include "smtpclient.h"

#include <QFileInfo>
#include <QByteArray>
#include <QEventLoop>

SmtpClient::SmtpError SmtpClient::connectToHost()
{
    switch (connectionType)
    {
    case TcpConnection:
        socket.reset(new QTcpSocket);
        socket->connectToHost(host, port);
        break;
    case SslConnection:
    case TlsConnection:
        {
            auto sslsock = std::make_unique<QSslSocket>();
            sslsock->connectToHostEncrypted(host, port);
            socket = std::move(sslsock);
        }
        break;
    }

    QObject::connect(socket.get(), SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
    QObject::connect(socket.get(), SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));
    QObject::connect(socket.get(), SIGNAL(readyRead()),
            this, SLOT(socketReadyRead()));

    // Tries to connect to server
    if (!socket->waitForConnected(connectionTimeout))
        return SmtpError::ConnectionTimeoutError;

    // Wait for the server's response
    if (auto error = waitForResponse())
        return error;

    // If the response code is not 220 (Service ready)
    // means that is something wrong with the server
    if (responseCode != 220)
        return SmtpError::ServerError;

    // Send a EHLO/HELO message to the server
    // The client's first command must be EHLO/HELO
    if (auto error = sendMessage("EHLO " + username))
        return error;

    // Wait for the server's response
    if (auto error = waitForResponse())
        return error;

    // The response code needs to be 250.
    if (responseCode != 250)
        return SmtpError::ServerError;

    if (connectionType == TlsConnection)
    {
        // send a request to start TLS handshake
        if (auto error = sendMessage("STARTTLS"))
            return error;

        // Wait for the server's response
        if (auto error = waitForResponse())
            return error;

        // The response code needs to be 220.
        if (responseCode != 220)
            return SmtpError::ServerError;

        QSslSocket* sslsock = dynamic_cast<QSslSocket*>(socket.get());

        sslsock->startClientEncryption();
        if (!sslsock->waitForEncrypted(connectionTimeout))
            return SmtpError::ConnectionTimeoutError;


        // Send ELHO one more time
        if (auto error = sendMessage("EHLO " + username))
            return error;

        // Wait for the server's response
        if (auto error = waitForResponse())
            return error;

        // The response code needs to be 250.
        if (responseCode != 250)
            return SmtpError::ServerError;
    }



    return SmtpClient::SmtpError::None;
}

SmtpClient::SmtpError SmtpClient::loginToServer()
{
    if (authMethod == AuthPlain)
    {
        // Sending command: AUTH PLAIN base64('\0' + username + '\0' + password)
        if (auto error = sendMessage("AUTH PLAIN " + QByteArray().append((char) 0).append(username).append((char) 0).append(password).toBase64()))
            return error;

        // Wait for the server's response
        if (auto error = waitForResponse())
            return error;

        // If the response is not 235 then the authentication was faild
        if (responseCode != 235)
            return SmtpError::AuthenticationFailedError;
    }
    else if (authMethod == AuthLogin)
    {
        // Sending command: AUTH LOGIN
        if (auto error = sendMessage("AUTH LOGIN"))
            return error;

        // Wait for 334 response code
        if (auto error = waitForResponse())
            return error;
        if (responseCode != 334)
            return SmtpError::AuthenticationFailedError;

        // Send the username in base64
        if (auto error = sendMessage(QByteArray().append(username).toBase64()))
            return error;

        // Wait for 334
        if (auto error = waitForResponse())
            return error;
        if (responseCode != 334)
            return SmtpError::AuthenticationFailedError;

        // Send the password in base64
        if (auto error = sendMessage(QByteArray().append(password).toBase64()))
            return error;

        // Wait for the server's responce
        if (auto error = waitForResponse())
            return error;

        // If the response is not 235 then the authentication was faild
        if (responseCode != 235)
            return SmtpError::AuthenticationFailedError;

    }
    return SmtpError::None;
}

SmtpClient::SmtpError SmtpClient::sendMail(const MimeMessage& email)
{
    // Send the MAIL command with the sender
    if (auto error = sendMessage("MAIL FROM: <" + email.getSender().getAddress() + ">"))
        return error;

    if (auto error = waitForResponse())
        return error;

    if (responseCode != 250)
        return SmtpError::ServerError;

    // Send RCPT command for each recipient
    QList<EmailAddress*>::const_iterator it, itEnd;
    // To (primary recipients)
    for (it = email.getRecipients().begin(), itEnd = email.getRecipients().end();
         it != itEnd; ++it)
    {

        if (auto error = sendMessage("RCPT TO:<" + (*it)->getAddress() + ">"))
            return error;
        if (auto error = waitForResponse())
            return error;
        if (responseCode != 250)
            return SmtpError::ServerError;
    }

    // Cc (carbon copy)
    for (it = email.getRecipients(MimeMessage::Cc).begin(), itEnd = email.getRecipients(MimeMessage::Cc).end();
         it != itEnd; ++it)
    {
        if (auto error = sendMessage("RCPT TO:<" + (*it)->getAddress() + ">"))
            return error;
        if (auto error = waitForResponse())
            return error;
        if (responseCode != 250)
            return SmtpError::ServerError;
    }

    // Bcc (blind carbon copy)
    for (it = email.getRecipients(MimeMessage::Bcc).begin(), itEnd = email.getRecipients(MimeMessage::Bcc).end();
         it != itEnd; ++it)
    {
        if (auto error = sendMessage("RCPT TO: <" + (*it)->getAddress() + ">"))
            return error;
        if (auto error = waitForResponse())
            return error;
        if (responseCode != 250)
            return SmtpError::ServerError;
    }

    // Send DATA command
    if (auto error = sendMessage("DATA"))
        return error;
    if (auto error = waitForResponse())
        return error;
    if (responseCode != 354)
        return SmtpError::ServerError;

    if (auto error = sendMessage(email.toString()))
        return error;

    // Send \r\n.\r\n to end the mail data
    if (auto error = sendMessage("."))
        return error;
    if (auto error = waitForResponse())
        return error;
    if (responseCode != 250)
        return SmtpError::ServerError;

    return SmtpError::None;

}

void SmtpClient::quit()
{
    if (auto error = sendMessage("QUIT") == SmtpError::None)
    {
        waitForResponse();
    }
    socket->disconnectFromHost();
    socket.reset();
}


SmtpClient::SmtpError SmtpClient::waitForResponse()
{
    do
    {
        if (!socket->waitForReadyRead(responseTimeout))
            return SmtpError::ResponseTimeoutError;

        while (socket->canReadLine())
        {
            // Save the server's response
            responseText = socket->readLine();

            // Extract the respose code from the server's responce (first 3 digits)
            responseCode = responseText.left(3).toInt();

            if (responseCode / 100 == 4)
                return SmtpError::ServerError;

            if (responseCode / 100 == 5)
                return SmtpError::ClientError;

            if (responseText[3] == ' ')
                return SmtpError::None;
        }
    } while (true);

    return SmtpError::None;
}

SmtpClient::SmtpError SmtpClient::sendMessage(const QString &text)
{
    socket->write(text.toUtf8() + "\r\n");
    if (! socket->waitForBytesWritten(sendMessageTimeout))
    {
        return SmtpError::SendDataTimeoutError;
    }
    return SmtpError::None;
}

void SmtpClient::socketStateChanged(QAbstractSocket::SocketState /*state*/)
{
}

void SmtpClient::socketError(QAbstractSocket::SocketError /*socketError*/)
{
}

void SmtpClient::socketReadyRead()
{
}

