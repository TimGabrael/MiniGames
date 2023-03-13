#pragma once
#include <qapplication>
#include "CommonCollection.h"
#include "Plugins/Client/PluginCommon.h"
#include "Network/NetClient.h"
#include <thread>
#undef max
#undef min

struct PluginInfo
{
	PluginInfo(const std::string& _id, uint32_t _sessionID) : id(_id),sessionID(_sessionID) {}
	std::string id;
	uint32_t sessionID;
};
struct SessionInput
{
	std::string name;
	std::string ip;
};

class MainWindow;
class MainApplication : public QApplication
{
public:
	MainApplication(int& argc, char** argv);
	~MainApplication();

	void SetNetworkingLobbyState();

	void CloseNetworkThread();
	void StartNetworkThread();

	void NetworkPoll();


	ApplicationData appData;
	std::vector<PluginInfo> serverPlugins;
	SessionInput input;
	QWidget* backgroundWidget = nullptr;
	MainWindow* mainWindow = nullptr;
	NetClient* client = nullptr;
	std::thread networkPollThread;

	bool isConnected = false;
	bool networkThreadShouldJoin = false;

	bool fullscreen = false;


	static MainApplication* GetInstance();
};

enum class ColorPalette
{
	DARK_BACKGROUND_COLOR,
	LIGHT_BACKGROUND_COLOR,

};
