#include "stdio.h"
#include "NinjamPlugin/NinjamPlugin.h"
#include "gui/MainWindow.h"
#include <iostream>
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


void NinjamPlugin::sendMetronomeEvent(NinjamPluginMetroEvent *metro) {
    if (metro) {
        std::cout << "Metronome event: BPM = " << metro->bpm
                  << ", BPI = " << metro->bpi
                  << ", Time = " << metro->time << std::endl;
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

