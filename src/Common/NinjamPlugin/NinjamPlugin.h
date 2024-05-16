#ifndef NINJAMPLUGIN_HH
#define NINJAMPLUGIN_HH
//#include "NinjamPluginPage.h"
#include "QObject"
#include "QUrl"
#include <QDesktopServices>
#include <QLibrary>
#include "gui/MainWindow.h"
#include "MainController.h"
#include "NinjamPlugin/NpSocket.h"
#include "NinjamPlugin/NpSocketServer.h"
#include "NinjamPlugin/NpWebSocketServer.h"
#include "NinjamPlugin/NpPipe.h"
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
    controller::MainController *mainController;
    NpWebSocketServer *socket_server;
    NpPipe *pipe_thread;
    void pluginLoadAll();
    void pluginLoad( QString path);
    void pluginScandir( QString path);

    
    void setMainWindow(MainWindow *win, controller::MainController *ctrl) {
	    mainWindow = win;
	    mainController = ctrl;
	    NpWebSocketServer *socket_server = new NpWebSocketServer(3003);
            pipe_thread = new NpPipe();
	    connect(pipe_thread, &NpPipe::sendData, socket_server, &NpWebSocketServer::broadcast);

	    QMenu *menu = win->menuBar()->addMenu("Plugins");
            QAction *action = menu->addAction("Video Sync");
     	    connect(action, &QAction::triggered, this, [this, action]() {
		QDesktopServices::openUrl(QUrl("https://ninbot.com:3108"));
        //	this->openNinjamPlugin(action->text());
    	    });
	    

    }


    void sendServerInfo();

    void sendMetronomeEvent(NinjamPluginMetroEvent *metro);
private:
//    NinjamPlugin() {} // Private constructor
    ~NinjamPlugin() {} // Private destructor

    
    // Delete copy constructor and assignment operation
    NinjamPlugin(const NinjamPlugin&) = delete;
    NinjamPlugin& operator=(const NinjamPlugin&) = delete;
};

#endif
