#ifndef NPSOCKET_H
#define NPSOCKET_H

#include <QObject>
#include <QTcpSocket>

class NpSocket : public QObject
{
    Q_OBJECT

public:
    explicit NpSocket(QObject *parent = nullptr);
    void connectToServer();
    QTcpSocket *socket;
    void write(QByteArray data) {
	   socket->write(data);
    }

private slots:
    void onConnected();
    void onReadyRead();

};

#endif // NPSOCKET_H
