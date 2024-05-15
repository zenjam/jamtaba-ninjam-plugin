#ifndef NPSOCKETSERVER_H
#define NPSOCKETSERVER_H

#include <QTcpServer>
#include <QTcpSocket>

class NpSocketServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit NpSocketServer(QObject *parent = nullptr);
    void startServer(int port);
    void broadcastMessage(const QByteArray &message);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QList<QTcpSocket *> clients; // Store connected clients
};

#endif // NPSOCKETSERVER_H
