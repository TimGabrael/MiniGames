#pragma once
#include <qapplication>
#include "CommonCollection.h"
#include "Plugins/PluginCommon.h"
#include "Network/NetClient.h"
#include <thread>
#undef max
#undef min

class MainWindow;
class MainApplication : public QApplication
{
public:
	MainApplication(int& argc, char** argv);
	~MainApplication();



	ApplicationData appData;
	QWidget* backgroundWidget = nullptr;
	MainWindow* mainWindow = nullptr;
	NetClient* client = nullptr;
	std::thread networkPollThread;

	bool isConnected = false;
	bool networkThreadShouldJoin = false;

	bool fullscreen;


	static MainApplication* GetInstance();
};

enum class ColorPalette
{
	DARK_BACKGROUND_COLOR,
	LIGHT_BACKGROUND_COLOR,

};