// WebEnginePage.cpp
#include "NinjamPluginPage.h"
#include "NinjamPluginWebBridge.h"
#include <QWebChannel>


NinjamPluginPage::NinjamPluginPage(QObject *parent)
    : QWebEnginePage(parent)
{
    connect(this, &NinjamPluginPage::featurePermissionRequested,
            this, &NinjamPluginPage::onFeaturePermissionRequested);

    QWebChannel *channel = new QWebChannel(this);
    this->setWebChannel(channel);

    // Create the bridge object
    //NinjamPluginWebBridge *bridge = new NinjamPluginWebBridge(this);
    bridge = new NinjamPluginWebBridge(this);
    channel->registerObject(QStringLiteral("webBridge"), bridge);
}

NinjamPluginPage::~NinjamPluginPage() {}

void NinjamPluginPage::onFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature) {

	  if(feature  == QWebEnginePage::MediaAudioCapture
                || feature == QWebEnginePage::MediaVideoCapture
                || feature == QWebEnginePage::MediaAudioVideoCapture)
            setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionGrantedByUser);
        else
            setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionDeniedByUser);
}
    // implementation
