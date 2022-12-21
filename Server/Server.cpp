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
	while (true)
	{
		server->Update(0.0f);
	}
	
	system("pause");
	//NetError err = NetError::OK;
	//UDPServerSocket socket;
	//
	//ServerData server;
	//server.sock = &socket;
	//server.plugin = nullptr;
	//Server_SetLobbyPlugin(&server);
	//
	//
	//
	//do
	//{
	//	err = socket.Create(DEBUG_IP, DEBUG_PORT);
	//	if (err != NetError::OK)
	//	{
	//		std::cout << "error: " << (uint32_t)err << std::endl;
	//	}
	//} while (err != NetError::OK);
	//
	//std::cout << "[SERVER] WAITING FOR CONNECTION" << std::endl;
	//
	//auto time = std::chrono::high_resolution_clock::now();
	//
	//while (true)
	//{
	//	auto now = std::chrono::high_resolution_clock::now();
	//	float dt = std::chrono::duration<float>(now - time).count();
	//	time = now;
	//	if (socket.Poll(dt))
	//	{
	//	}
	//}

	return 0;
}