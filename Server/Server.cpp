#include <iostream>
#include "CommonCollection.h"


void PacketCB(void* userData, Packet* packet)
{
	TestPacket* pack = (TestPacket*)packet->body.data();
	std::cout << "pack: " << pack->buf << std::endl;
}

int main()
{
	NetError err = NetError::OK;
	TCPServerSocket server;

	server.SetPacketCallback(PacketCB);

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

	}

	return 0;
}