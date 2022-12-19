#include "Application.h"
#include <qfile.h>
#include "UtilFuncs.h"
#include "CustomWidgets/InfoPopup.h"


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
}
void __stdcall NetDisconnectCallback(NetClient* c, ClientConnection* conn)
{
	if (c->GetSelf() == conn)
	{
		SafeAsyncUI([](MainWindow* main) {
			auto rect = main->geometry();
			InfoPopup* popUp = new InfoPopup(main, "FAILED TO CONNECT TO SERVER", QPoint(rect.width() / 2, rect.height() - 100), 20, 0xFFFF0000, 3000);
		});
	}
}


MainApplication* g_mainApplicationInstance = nullptr;
MainApplication::MainApplication(int& argc, char** argv) : QApplication(argc, argv), networkPollThread(std::bind(NetworkPollFunction, this))
{
	g_mainApplicationInstance = this;
	InitNetworking();

	client = NetClient::Create();
	client->SetUserData(this);
	client->SetJoinCallback((ClientJoinCallbackFunction)NetJoinCallback);
	client->SetDisconnectCallback((ClientJoinCallbackFunction)NetDisconnectCallback);


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