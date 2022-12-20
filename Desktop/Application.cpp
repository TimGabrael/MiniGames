#include "Application.h"
#include <qfile.h>
#include "UtilFuncs.h"
#include "CustomWidgets/InfoPopup.h"
#include "Frames/LobbyFrame.h"

void NetworkPollFunction(MainApplication* app)
{
	auto time = std::chrono::high_resolution_clock::now();
	while (!app->networkThreadShouldJoin)
	{
		if (app->client)
		{
			app->client->Poll();
			NetRunCallbacks();
		}
		std::this_thread::sleep_for(std::chrono::duration<float>(0.1f));
	}
}

void __stdcall NetJoinCallback(NetClient* c, ClientConnection* conn)
{
	MainApplication* app = (MainApplication*)c->GetUserData();
	ClientData* cl = nullptr;
	if(conn == c->GetSelf())
	{
		cl = &app->appData.localPlayer;
	}
	else
	{
		for (int i = 0; i < app->appData.players.size(); i++)
		{
			if (app->appData.players.at(i).clientID == conn->id)
			{
				cl = &app->appData.players.at(i);
				break;
			}
		}
	}
	if (cl)
	{
		app->appData.localPlayer.clientID = conn->id;
		app->appData.localPlayer.name = conn->name;
		app->appData.localPlayer.isAdmin = conn->isAdmin;
	}
	else
	{
		ClientData data;
		data.clientID = conn->id;
		data.isAdmin = conn->isAdmin;
		data.name = conn->name;
		app->appData.players.push_back(data);
	}
	
	if (app->mainWindow && app->mainWindow->GetState() == MAIN_WINDOW_STATE::STATE_LOBBY)
	{
		app->mainWindow->SetState(MAIN_WINDOW_STATE::STATE_INVALID);
		app->mainWindow->SetState(MAIN_WINDOW_STATE::STATE_LOBBY);
	}

}
void __stdcall NetDisconnectCallback(NetClient* c, ClientConnection* conn)
{
	MainApplication* app = (MainApplication*)c->GetUserData();
	if (c->GetSelf() == conn)
	{
		app->appData.localPlayer.clientID = 0;
		app->appData.localPlayer.isAdmin = false;
		app->appData.localPlayer.name = "";
		SafeAsyncUI([](MainWindow* main) {
			auto rect = main->geometry();
			InfoPopup* popUp = new InfoPopup(main, "FAILED TO CONNECT TO SERVER", QPoint(rect.width() / 2, rect.height() - 100), 20, 0xFFFF0000, 3000);
		});
	}
	else
	{
		for (int i = 0; i < app->appData.players.size(); i++)
		{
			if (app->appData.players.at(i).clientID == conn->id)
			{
				app->appData.players.erase(app->appData.players.begin() + i);
				break;
			}
		}
	}
	
	if (app->mainWindow && app->mainWindow->GetState() == MAIN_WINDOW_STATE::STATE_LOBBY)
	{
		app->mainWindow->SetState(MAIN_WINDOW_STATE::STATE_INVALID);
		app->mainWindow->SetState(MAIN_WINDOW_STATE::STATE_LOBBY);
	}
	

}
bool __stdcall NetSetStateCallback(NetClient* c, base::ServerSetState* state, int packetSize)
{
	if (state->has_plugin_id())
	{

	}
	else
	{
		if (state->state() == (int32_t)ClientState::LOBBY)
		{
			MainApplication* app = (MainApplication*)c->GetUserData(); 
			if(app->mainWindow) app->mainWindow->SetState(MAIN_WINDOW_STATE::STATE_LOBBY);
		}
	}
	return true;
}

static MainApplication* g_mainApplicationInstance = nullptr;
MainApplication::MainApplication(int& argc, char** argv) : QApplication(argc, argv), networkPollThread(std::bind(NetworkPollFunction, this))
{
	g_mainApplicationInstance = this;

	appData.platform = PLATFORM_MASK_DESKTOP;
	appData.localPlayer.clientID = 0;
	appData.localPlayer.isAdmin = false;
	appData.localPlayer.name = "";


	InitNetworking();


	client = NetClient::Create();
	client->SetUserData(this);
	client->SetCallback((ClientPacketFunction)NetSetStateCallback, Server_SetState);
	client->SetClientInfoCallback((ClientInfoCallbackFunction)NetJoinCallback);
	client->SetDisconnectCallback((ClientDisconnectCallbackFunction)NetDisconnectCallback);


}
MainApplication::~MainApplication()
{
	networkThreadShouldJoin = true;
	if(networkPollThread.joinable()) networkPollThread.join();

}
MainApplication* MainApplication::GetInstance()
{
	return g_mainApplicationInstance;
}