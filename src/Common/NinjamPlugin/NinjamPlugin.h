#ifndef NINJAMPLUGIN_HH
#define NINJAMPLUGIN_HH
//#include "NinjamPluginPage.h"
#include "QObject"
#include "QUrl"
#include <QDesktopServices>
#include <QLibrary>
#include "gui/MainWindow.h"
//#include "QWebEngineView"

class NinjamPluginInterface {
public:
    virtual ~NinjamPluginInterface() {}
    virtual void processEvent(const QString &eventJson) = 0;
};

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
//    void  addView(QString,  QWebEngineView* view, NinjamPluginPage *page);
    MainWindow *mainWindow;
    void pluginLoadAll();
    void pluginLoad( QString path);
    void pluginScandir( QString path);
    void setMainWindow(MainWindow *win) {
	    mainWindow = win;
	    QMenu *menu = win->menuBar()->addMenu("Plugins");
            QAction *action = menu->addAction("Video Sync");
     	    connect(action, &QAction::triggered, this, [this, action]() {
		QDesktopServices::openUrl(QUrl("https://www.ninbot.com"));
        //	this->openNinjamPlugin(action->text());
    	    });
	    

    }
    void sendMetronomeEvent(NinjamPluginMetroEvent *metro);
private:
//    NinjamPlugin() {} // Private constructor
    ~NinjamPlugin() {} // Private destructor

    
    // Delete copy constructor and assignment operation
    NinjamPlugin(const NinjamPlugin&) = delete;
    NinjamPlugin& operator=(const NinjamPlugin&) = delete;
};

#endif
