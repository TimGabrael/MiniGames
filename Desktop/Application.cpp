#include "Application.h"
#include <qfile.h>
#include <qregularexpression.h>


void NetworkPollFunction(MainApplication* app)
{
	auto time = std::chrono::high_resolution_clock::now();
	while (!app->networkThreadShouldJoin)
	{
		auto now = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float>(now - time).count();
		time = now;
		//if (app->socket.Poll(dt))
		//{
		//
		//}

		std::this_thread::sleep_for(std::chrono::duration<float>(0.1f));
	}
}



MainApplication* g_mainApplicationInstance = nullptr;
MainApplication::MainApplication(int& argc, char** argv) : QApplication(argc, argv), networkPollThread(std::bind(NetworkPollFunction, this))
{
	g_mainApplicationInstance = this;
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