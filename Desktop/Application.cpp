#include "Application.h"
#include <qfile.h>
#include <qregularexpression.h>


void NetworkPollFunction(MainApplication* app)
{
	auto time = std::chrono::high_resolution_clock::now();
	while (!app->networkThreadShouldJoin)
	{
		if (app->client)
		{
			app->client->Poll();
		}
		std::this_thread::sleep_for(std::chrono::duration<float>(0.1f));
	}
}



MainApplication* g_mainApplicationInstance = nullptr;
MainApplication::MainApplication(int& argc, char** argv) : QApplication(argc, argv), networkPollThread(std::bind(NetworkPollFunction, this))
{
	g_mainApplicationInstance = this;
	InitNetworking();



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