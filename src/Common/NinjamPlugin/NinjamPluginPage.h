// NinjamPluginPage.h
#ifndef WEBENGINEPAGE_H
#define WEBENGINEPAGE_H

#include <QWebEnginePage>
#include "NinjamPluginWebBridge.h"

class NinjamPluginPage : public QWebEnginePage {
    Q_OBJECT
public:
    explicit NinjamPluginPage(QObject *parent = nullptr);
    virtual ~NinjamPluginPage();
    NinjamPluginWebBridge *bridge;
private slots:
    void onFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature);
};

#endif // WEBENGINEPAGE_H
