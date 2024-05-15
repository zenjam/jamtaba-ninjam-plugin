#include "NpSocket.h"

NpSocket::NpSocket(QObject *parent) : QObject(parent)
{
    socket = new QTcpSocket(this);

    // Connecting signals and slots for socket communication
    connect(socket, &QTcpSocket::connected, this, &NpSocket::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &NpSocket::onReadyRead);
}

void NpSocket::connectToServer()
{
    socket->connectToHost("ninbot.com", 3109);
}

void NpSocket::onConnected() {
    qDebug() << "Connected to the server!";
    // Optionally send a message upon connection
    socket->write("Hello from Qt client");
}

void NpSocket::onReadyRead() {
    QByteArray data = socket->readAll();
    qDebug() << "Data received from server:" << data;
}

