#ifndef NINJAMPLUGIN_HH
#define NINJAMPLUGIN_HH
#include "NinjamPluginPage.h"
#include "QObject"
#include "QWebEngineView"

class NinjamPluginMetroEvent
{
public:
    int bpm;
    int bpi;
    float time;
};

class NinjamPlugin : public QObject {
    Q_OBJECT

public:
	        NinjamPlugin(QObject *parent = nullptr) : QObject(parent) {}

    static NinjamPlugin& getInstance(); // Singleton access
    void  addView(QString,  QWebEngineView* view, NinjamPluginPage *page);
    void sendMetronomeEvent(NinjamPluginMetroEvent *metro);
private:
//    NinjamPlugin() {} // Private constructor
    ~NinjamPlugin() {} // Private destructor

    
    // Delete copy constructor and assignment operation
    NinjamPlugin(const NinjamPlugin&) = delete;
    NinjamPlugin& operator=(const NinjamPlugin&) = delete;
};

#endif
