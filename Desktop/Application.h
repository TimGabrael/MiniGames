#pragma once
#include <qapplication>
#include "CommonCollection.h"
#include "Plugins/PluginCommon.h"

class MainApplication : public QApplication
{
public:
	MainApplication(int& argc, char** argv);
	~MainApplication();



	ApplicationData appData;
	TCPSocket socket;
	QWidget* backgroundWidget = nullptr;

	QString username;
	QString lobbyname;
	bool isConnected = false;

	bool fullscreen;


	static MainApplication* GetInstance();
};

enum class ColorPalette
{
	DARK_BACKGROUND_COLOR,
	LIGHT_BACKGROUND_COLOR,

};