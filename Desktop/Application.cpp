#include "Application.h"
#include <qfile.h>
#include "UtilFuncs.h"
#include "CustomWidgets/InfoPopup.h"
#include "Frames/LobbyFrame.h"
#include "util/FileStorage.h"
#include "util/PluginLoader.h"
#include "Frames/PluginFrame.h"

static void NetworkPollFunction(MainApplication* app)
{
	while (!app->networkThreadShouldJoin)
	{
		if (app->client)
		{
			app->NetworkPoll();
		}
		std::this_thread::sleep_for(std::chrono::duration<float>(0.1f));
	}
}

static void __stdcall NetJoinCallback(ApplicationData* c, ClientConnection* conn)
{
	MainApplication* app = MainApplication::GetInstance();
	ClientData* cl = nullptr;
	if(conn == c->net->GetSelf())
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
		cl->clientID = conn->id;
		cl->name = conn->name;
		cl->isAdmin = conn->isAdmin;
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
static void __stdcall NetDisconnectCallback(ApplicationData* c, ClientConnection* conn)
{
	MainApplication* app = MainApplication::GetInstance();
	if (c->net->GetSelf() == conn)
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
static bool __stdcall NetSetStateCallback(ApplicationData* c, base::ServerSetState* state, int packetSize)
{
	MainApplication* app = MainApplication::GetInstance();
	uint16_t pluginID = state->has_plugin_id() ? state->plugin_id() : INVALID_ID;
	if (pluginID != INVALID_ID)
	{
		const std::vector<PluginClass*>& pls = GetPlugins();
		for (int i = 0; i < app->serverPlugins.size(); i++)
		{
			const PluginInfo& plInfo = app->serverPlugins.at(i);
			if (plInfo.sessionID == state->plugin_id())
			{
				for (int j = 0; j < pls.size(); j++)
				{
					PLUGIN_INFO info = pls.at(j)->GetPluginInfos();
					std::string infoStr(info.ID, 19);
					if (infoStr == plInfo.id)
					{

						PluginFrame::activePlugin = pls.at(j);
						PluginFrame::activePluginID = plInfo.sessionID;
						SafeAsyncUI([](MainWindow* wnd) {
							MainApplication* app = MainApplication::GetInstance();
							app->CloseNetworkThread();
							app->mainWindow->SetState(MAIN_WINDOW_STATE::STATE_PLUGIN);
		
							base::ClientState response;
							response.set_state((int32_t)AppState::PLUGIN);
							response.set_plugin_session_id(PluginFrame::activePluginID);
							app->client->SendData(Client_State, &response, SendFlags::Send_Reliable);
						});
						break;
					}
				}
				break;
			}
		}
	}
	else
	{
		if (state->state() == (int32_t)AppState::LOBBY)
		{
			SafeAsyncUI([](MainWindow* wnd) {
				MainApplication* app = MainApplication::GetInstance();
				wnd->SetState(MAIN_WINDOW_STATE::STATE_LOBBY);
				app->SetNetworkingLobbyState();
			});

		}
	}
	state->Clear();
	return true;
}
static bool __stdcall NetPluginCallback(ApplicationData* c, base::ServerPlugin* plugin, int packetSize)
{
	MainApplication* app = MainApplication::GetInstance();
	std::string id = plugin->data().id();
	int32_t sessionID = plugin->data().session_id();

	app->serverPlugins.emplace_back(id, sessionID);

	plugin->Clear();
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
	appData.plugin = nullptr;

	IniFile file;
	if (!file.LoadFile("settings.ini"))
	{
		file.SetStringValue("IP", "192.168.2.100:20000");
		file.Store("settings.ini");
	}
	else
	{
		file.GetStringValue("IP", input.ip);
	}

	InitNetworking();

	client = NetClient::Create();
	appData.net = client;
	client->appData = &appData;
	client->SetCallback((ClientPacketFunction)NetSetStateCallback, Server_SetState);
	client->SetCallback((ClientPacketFunction)NetPluginCallback, Server_Plugin);
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


static bool __stdcall NetLobbyVoteCallback(ApplicationData* c, base::ServerLobbyVote* vote, int packetSize)
{
	uint16_t client = vote->client_id();
	uint16_t plugin = vote->plugin_id();

	bool found = false;
	for (int i = 0; i < LobbyFrame::data.votes.size(); i++)
	{
		if (LobbyFrame::data.votes.at(i).clientID == client)
		{
			if (plugin == 0xFFFF) LobbyFrame::data.votes.erase(LobbyFrame::data.votes.begin() + i);
			else LobbyFrame::data.votes.at(i).pluginID = plugin;
			found = true;
			break;
		}
	}

	if (!found) LobbyFrame::data.votes.push_back({ client, plugin });


	SafeAsyncUI([](MainWindow* wnd) {
		if (wnd->GetState() == MAIN_WINDOW_STATE::STATE_LOBBY)
		{
			LobbyFrame* frame = (LobbyFrame*)wnd->stateWidget;
			frame->UpdateFromData();
		}
	});

	return true;
}
static bool __stdcall NetLobbyTimerCallback(ApplicationData* c, base::ServerLobbyTimer* timer, int packetSize)
{
	MainApplication* app = MainApplication::GetInstance();
	
	LobbyFrame* frame = (LobbyFrame*)app->mainWindow->stateWidget;
	frame->SetTimer(timer->time());

	return true;
}


void MainApplication::SetNetworkingLobbyState()
{
	client->SetLobbyDeserializers();
	client->SetCallback((ClientPacketFunction)NetLobbyVoteCallback, Server_LobbyVote);
	client->SetCallback((ClientPacketFunction)NetLobbyTimerCallback, Server_LobbyTimer);

	base::ClientState response;
	response.set_state((int32_t)AppState::LOBBY);
	client->SendData(Client_State, &response, SendFlags::Send_Reliable);
}
void MainApplication::CloseNetworkThread()
{
	if (networkPollThread.joinable())
	{
		networkThreadShouldJoin = true;
		networkPollThread.join();
	}
}
void MainApplication::StartNetworkThread()
{
	if (networkPollThread.joinable()) return;
	networkThreadShouldJoin = false;
	networkPollThread = std::thread(std::bind(NetworkPollFunction, this));
}
void MainApplication::NetworkPoll()
{
	client->Poll();
	NetRunCallbacks();
}
