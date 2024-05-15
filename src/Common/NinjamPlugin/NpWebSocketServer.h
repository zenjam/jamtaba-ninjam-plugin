#ifndef NPWEBSOCKETSERVER_H
#define NPWEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>

class NpWebSocketServer : public QObject
{
    Q_OBJECT
public:
    explicit NpWebSocketServer(quint16 port, QObject *parent = nullptr);
    ~NpWebSocketServer();

    void broadcast(const QByteArray &data); // Method to send binary data to all clients

private slots:
    void onNewConnection();
    void processMessage(const QString &message);
    void processBinaryMessage(const QByteArray &message); // If you need to handle binary messages
    void socketDisconnected();

private:
    QWebSocketServer *m_webSocketServer;
    QList<QWebSocket *> m_clients; // List of connected clients
};

#endif // NPWEBSOCKETSERVER_H
