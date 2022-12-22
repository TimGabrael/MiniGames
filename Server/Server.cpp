#include <iostream>
#include "CommonCollection.h"
#include <chrono>
#include "Network/NetServer.h"
#include "Network/NetCommon.h"


int main()
{
	InitNetworking();

	ServerData* server = new ServerData(DEBUG_IP, DEBUG_PORT);

	
	std::cout << "waiting for clients\n";
	auto now = std::chrono::high_resolution_clock::now();
	while (true)
	{
		const auto later = std::chrono::high_resolution_clock::now();
		const float dt = std::chrono::duration<float>(later - now).count();
		now = later;
		server->Update(dt);
	}
	
	system("pause");

	return 0;
}