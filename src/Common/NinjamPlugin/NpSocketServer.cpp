#include "NpSocketServer.h"
#include <QDebug>

NpSocketServer::NpSocketServer(QObject *parent)
    : QTcpServer(parent)
{
}

void NpSocketServer::startServer(int port)
{
    if (!listen(QHostAddress::Any, port)) {
        qDebug() << "Server could not start!";
    } else {
        qDebug() << "Server started on port:" << port;
    }
}

void NpSocketServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *client = new QTcpSocket(this);
    client->setSocketDescriptor(socketDescriptor);
    clients.append(client);

    connect(client, &QTcpSocket::readyRead, this, &NpSocketServer::onReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &NpSocketServer::onDisconnected);

    qDebug() << "New connection from" << client->peerAddress().toString();
}

void NpSocketServer::onReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (client) {
        QByteArray data = client->readAll();
        qDebug() << "Received:" << data;

        // Optionally echo back the received data to all clients
        broadcastMessage(data);
    }
}

void NpSocketServer::onDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (client) {
        qDebug() << "Client disconnected:" << client->peerAddress().toString();
        clients.removeAll(client);
        client->deleteLater();
    }
}

void NpSocketServer::broadcastMessage(const QByteArray &message)
{
    for (QTcpSocket *client : clients) {
        if (client->state() == QTcpSocket::ConnectedState) {
            client->write(message);
        }
    }
}
