#ifndef NINJAMPLUGINWEBBRIDGE_H
#define NINJAMPLUGINWEBBRIDGE_H

#include <QObject>
#include <QString>
#include <QWebChannel>

class NinjamPluginWebBridge : public QObject {
    Q_OBJECT

public:
    explicit NinjamPluginWebBridge(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void fromJavaScript(const QString &message) {
        qDebug() << "Message from JavaScript:" << message;
    }

signals:
    void sendToJavaScript(const QString &message);
};

#endif // NINJAMPLUGINWEBBRIDGE_H
