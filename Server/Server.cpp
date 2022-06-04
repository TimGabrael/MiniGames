#include <iostream>
#include "CommonCollection.h"

int main()
{
	NetError err = NetError::OK;
	TCPServerSocket server;

	do
	{
		err = server.Create(DEBUG_IP, DEBUG_PORT);
		if (err != NetError::OK)
		{
			std::cout << "error: " << (uint32_t)err << std::endl;
		}
	} while (err != NetError::OK);

	std::cout << "[SERVER] WAITING FOR CONNECTION" << std::endl;

	while (true)
	{
		server.AcceptConnection();
		std::cout << "her" << std::endl;
	}

	return 0;
}