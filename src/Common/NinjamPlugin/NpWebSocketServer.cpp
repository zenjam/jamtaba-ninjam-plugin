#include "NpWebSocketServer.h"
#include <QDebug>

NpWebSocketServer::NpWebSocketServer(quint16 port, QObject *parent) :
    QObject(parent),
    m_webSocketServer(new QWebSocketServer(QStringLiteral("WebSocket Server"),
                                           QWebSocketServer::NonSecureMode, this))
{
    if (m_webSocketServer->listen(QHostAddress::Any, port)) {
        qDebug() << "WebSocket server started on port" << port;
        connect(m_webSocketServer, &QWebSocketServer::newConnection,
                this, &NpWebSocketServer::onNewConnection);
    } else {
        qDebug() << "Failed to start WebSocket server!";
    }
}

NpWebSocketServer::~NpWebSocketServer()
{
    m_webSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void NpWebSocketServer::onNewConnection()
{
    QWebSocket *socket = m_webSocketServer->nextPendingConnection();

    connect(socket, &QWebSocket::textMessageReceived, this, &NpWebSocketServer::processMessage);
    connect(socket, &QWebSocket::binaryMessageReceived, this, &NpWebSocketServer::processBinaryMessage);
    connect(socket, &QWebSocket::disconnected, this, &NpWebSocketServer::socketDisconnected);

    m_clients << socket;
    qDebug() << "New WebSocket connection established.";
}

void NpWebSocketServer::processMessage(const QString &message)
{
    QWebSocket *sender = qobject_cast<QWebSocket *>(QObject::sender());
    qDebug() << "Message received:" << message;
}

void NpWebSocketServer::processBinaryMessage(const QByteArray &message)
{
    // Here, you can handle binary data received from clients
    qDebug() << "Binary message received.";
}
void NpWebSocketServer::socketDisconnected()
{
    QWebSocket *client = qobject_cast<QWebSocket *>(QObject::sender());
    if (client) {
        qDebug() << "WebSocket connection closed from" << client->peerAddress().toString();
        m_clients.removeAll(client);
        client->deleteLater();
    }
}
void NpWebSocketServer::broadcast(const QByteArray &data)
{
    for (QWebSocket *client : qAsConst(m_clients)) {
        client->sendTextMessage(data);
        //client->sendBinaryMessage(data);
    }
}
