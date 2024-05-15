#ifndef NPPIPE_H
#define NPPIPE_H

#include <QThread>
#include <QDebug>
#include <QObject>
#include <QByteArray>

class NpPipe : public QThread  

{
    Q_OBJECT

public:
    using QThread::QThread; // Inherit constructors
    void sendStr(const QByteArray &str)  {
	qDebug() << "Sending str to pipe" ;
	emit sendData(str);
    }

signals:
    void sendData(const QByteArray &data) ;
    
};

#endif // NPPIPE_H

