#include <iostream>
#include "CommonCollection.h"
#include "Room.h"
#include <chrono>



int main()
{
	NetError err = NetError::OK;
	UDPServerSocket server;

	

	
	do
	{
		err = server.Create(DEBUG_IP, DEBUG_PORT);
		if (err != NetError::OK)
		{
			std::cout << "error: " << (uint32_t)err << std::endl;
		}
	} while (err != NetError::OK);

	std::cout << "[SERVER] WAITING FOR CONNECTION" << std::endl;

	auto time = std::chrono::high_resolution_clock::now();

	while (true)
	{
		auto now = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float>(now - time).count();
		time = now;
		if (server.Poll(dt))
		{
		}
	}

	return 0;
}