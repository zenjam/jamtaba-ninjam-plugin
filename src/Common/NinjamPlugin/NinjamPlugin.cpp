#include "stdio.h"
#include "NinjamPlugin/NinjamPlugin.h"
#include "NinjamPlugin/NpSocket.h"
#include "NinjamPlugin/NpPipe.h"
#include "gui/MainWindow.h"
#include <iostream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include "ninjam/client/Service.h"

    struct ninjam_plugin {
	    QString action;
    };
	QList<ninjam_plugin> plugins;
	/*
void  NinjamPlugin::addView(QString action, QWebEngineView *view, NinjamPluginPage * page) {
	ninjam_plugin plug = { action, view, page };
	plugins.append(plug);
}
*/

NinjamPlugin& NinjamPlugin::getInstance() {
    static NinjamPlugin instance; // Guaranteed to be destroyed and instantiated on first use
    return instance;
}
void NinjamPlugin::pluginLoadAll( ) {
    QString executablePath = QCoreApplication::applicationDirPath();
    QString pluginPath = executablePath + "/plugins/";  
    pluginScandir(pluginPath);
}

void NinjamPlugin::pluginLoad( QString path) {
    QLibrary library(path);
    bool loaded = library.load();

    if (loaded) {
    // The library was loaded successfully
    // Proceed with using the plugin
    } else {
    // Handle the error
        qDebug() << "Error loading plugin:" << library.errorString();
    }

}
void NinjamPlugin::pluginScandir( QString path)
{

}

void NinjamPlugin::sendServerInfo() {
   QJsonObject jsonObj;
   jsonObj["event"] = "server";
   auto service = mainController->getNinjamService();
   QString username =  mainController->getUserName();
   jsonObj["username"] = username;
   //     std::cout << "Username = " << username;
   if (service) {
         auto serverInfo = service->getCurrentServer();
         if (serverInfo) {
		 /*
	      for (const auto& user : serverInfo->getUsers()) {
                   ninjam::client::maskIpInUserFullName(user.getFullName());
         	   //std::cout << "On server = " << user.getFullName();
              }
	      */
	      //std::cout << "Host = " +  serverInfo->getHostName();
	      //std::cout << "Port = " +  serverInfo->getPort();
	      jsonObj["host"] = serverInfo->getHostName();
	      jsonObj["port"] = serverInfo->getPort();
	 }
   }
   QJsonDocument jsonDoc(jsonObj);
   QByteArray ev = jsonDoc.toJson();
   pipe_thread->sendStr(ev);
}

void NinjamPlugin::sendMetronomeEvent(NinjamPluginMetroEvent *metro) {
    if (metro) {
        std::cout << "New Metronome event: BPM = " << metro->bpm
                  << ", BPI = " << metro->bpi
                  << ", Time = " << metro->time << std::endl;
 	QJsonObject jsonObj;
	jsonObj["event"] = "metro";
	jsonObj["bpi"] = metro->bpi;
	jsonObj["bpm"] = metro->bpm;
	jsonObj["beat"] = metro->time;  // Assuming metro->time is a double; it will be handled properly.
	QJsonDocument jsonDoc(jsonObj);
	QByteArray ev = jsonDoc.toJson();
	pipe_thread->sendStr(ev);
		
	sendServerInfo();

/*
	QString evstr;
 	evstr.sprintf("{ \"event\":\"metro\", \"bpi\":%d, \"bpm\":%d, \"beat\":%g }", metro->bpi, metro->bpm, metro->time);
	QByteArray ev = evstr.toUtf8();
	*/
/*
	char ev[100];
		sprintf(ev, "{ \"event\":\"metro\", \"bpi\":%d, \"bpm\":%d, \"beat\":%g }", metro->bpi, metro->bpm, metro->time);
		QByteArray data = "Metro test";
		*/
		pipe_thread->sendStr(ev);
		QString str = "Metro Test";
		//test_socket->socket->write("Metro Test");
		//test_socket->socket->write(ev);
		//test_socket->socket->write(str);
	return;
#
	foreach (const ninjam_plugin &plugin, plugins) {
		std::cout << "Got a plugin";
		
	//	plugin.page.bridge.sendToJavaScript("Metro:" + metro->bpm + ":" + metro->bpi +":" + metro.time;
	        char ev[100];
		sprintf(ev, "{ \"event\":\"metro\", \"bpi\":%d, \"bpm\":%d, \"beat\":%g }", metro->bpi, metro->bpm, metro->time);
//		plugin.page->bridge->sendToJavaScript(ev );
	}

        std::cout << "Metronome event: BPM = " << metro->bpm
                  << ", BPI = " << metro->bpi
                  << ", Time = " << metro->time << std::endl;
    } else {
        std::cout << "Error: Null metronome event received." << std::endl;
    }
}

