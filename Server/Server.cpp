#include <iostream>
#include "CommonCollection.h"
#include "Network/Messages/join.pb.h"

void PacketCB(void* userData, Packet* packet)
{
	// uint8_t* pack = (uint8_t*)packet->body.data();
	// std::cout << "pack: " << pack->buf << std::endl;

	if (packet->header.type == (uint32_t)PacketID::JOIN)
	{
		Base::Join j;
		j.ParseFromArray(packet->body.data(), packet->body.size());
		LOG("JOIN MESSAGE: %s\n\t%s\n\t%s\n", j.name().c_str(), j.serverid().c_str(), j.id().c_str());
	}
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