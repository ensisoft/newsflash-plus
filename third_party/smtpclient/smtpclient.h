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

#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <QObject>
#include <QtNetwork/QSslSocket>
#include <memory>

#include "mimemessage.h"
#include "smtpexports.h"

class SMTP_EXPORT SmtpClient : public QObject
{
    Q_OBJECT
public:
    enum AuthMethod
    {
        AuthPlain,
        AuthLogin
    };

    enum SmtpError
    {
        None,
        ConnectionTimeoutError,
        ResponseTimeoutError,
        SendDataTimeoutError,
        AuthenticationFailedError,
        ServerError,    // 4xx smtp error
        ClientError     // 5xx smtp error
    };

    enum ConnectionType
    {
        TcpConnection,
        SslConnection,
        TlsConnection       // STARTTLS
    };

    const QString& getHost() const
    { return host; }
    void setHost(const QString &host)
    { this->host = host; }

    int getPort() const
    { return port; }
    void setPort(int port)
    { this->port = port; }

    ConnectionType getConnectionType() const
    { return connectionType; }
    void setConnectionType(ConnectionType ct)
    { this->connectionType = ct; }

    const QString& getUsername() const
    { return username; }
    void setUsername(const QString& username)
    { this->username = username; }

    const QString& getPassword() const
    { return password; }
    void setPassword(const QString& password)
    { this->password = password; }

    SmtpClient::AuthMethod getAuthMethod() const
    { return authMethod; }
    void setAuthMethod(AuthMethod method)
    { this->authMethod = method; }

    const QString & getResponseText() const
    { return responseText; }
    int getResponseCode() const
    { return responseCode; }

    int getConnectionTimeout() const
    { return connectionTimeout; }
    void setConnectionTimeout(int msec)
    { connectionTimeout = msec; }

    int getResponseTimeout() const
    { return responseTimeout; }
    void setResponseTimeout(int msec)
    { responseTimeout = msec; }

    int getSendMessageTimeout() const
    { return sendMessageTimeout; }
    void setSendMessageTimeout(int msec)
    { sendMessageTimeout = msec; }

    SmtpError connectToHost();
    SmtpError loginToServer();
    SmtpError sendMail(const MimeMessage& email);
    void quit();
    void cancel();

private slots:
    void socketStateChanged(QAbstractSocket::SocketState state);
    void socketError(QAbstractSocket::SocketError error);
    void socketReadyRead();

private:
    SmtpError waitForResponse();
    SmtpError sendMessage(const QString &text);

private:
    std::unique_ptr<QTcpSocket> socket;

    QString host;
    unsigned int port = 465;
    ConnectionType connectionType = ConnectionType::SslConnection;

    QString username;
    QString password;
    AuthMethod authMethod = AuthMethod::AuthLogin;

    int connectionTimeout  = 5000;
    int responseTimeout    = 5000;
    int sendMessageTimeout = 60000;

    QString responseText;
    int responseCode = 0;
private:
};

#endif // SMTPCLIENT_H
