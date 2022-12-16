#pragma once
#include <qapplication>
#include "CommonCollection.h"
#include "Plugins/PluginCommon.h"
#undef max
#undef min

class MainWindow;
class MainApplication : public QApplication
{
public:
	MainApplication(int& argc, char** argv);
	~MainApplication();



	ApplicationData appData;
	UDPSocket socket;
	QWidget* backgroundWidget = nullptr;
	MainWindow* mainWindow = nullptr;

	std::thread networkPollThread;
	std::vector<Server::PluginData> serverPlugins;

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