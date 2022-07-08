#include <iostream>
#include "CommonCollection.h"
#include "Network/Messages/join.pb.h"
#include "Room.h"

void PacketCB(void* userData, Connection* conn, Packet* packet)
{
	LockResourceAccess();

	TCPServerSocket* server = (TCPServerSocket*)userData;
	// uint8_t* pack = (uint8_t*)packet->body.data();
	// std::cout << "pack: " << pack->buf << std::endl;

	if (packet->header.type == (uint32_t)PacketID::JOIN)
	{
		
		Base::JoinRequest j;
		j.ParseFromArray(packet->body.data(), packet->body.size());
		
		if (j.has_id() && j.id().size() == 16)
		{

			//AddClientInfo(socket, )
		}
		else
		{
			ClientInfo* info = AddClientInfo(conn, GetIPAddress(conn->socket), j.info().client().name(), j.info().client().listengroup() & ~(ADMIN_GROUP_MASK));
			
			auto err = ClientJoinRoom(info, j.info().serverid());
			LOG("%s, %s\n", j.info().client().name().c_str(), j.info().serverid().c_str());

			
			Base::JoinResponse response;
			response.set_error(err);
			response.set_id(info->id, 16);
			response.mutable_info()->mutable_client()->set_name(info->name);
			response.mutable_info()->set_serverid(j.info().serverid());
			response.mutable_info()->mutable_client()->set_listengroup(info->groupMask);
			std::string serialized = response.SerializeAsString();
			conn->SendData(PacketID::JOIN, serialized.size(), serialized.data());

		}
	}
	if (packet->header.type == (uint32_t)PacketID::CREATE)
	{
		Base::CreateRequest req;
		req.ParseFromArray(packet->body.data(), packet->body.size());
		
		ClientInfo* info = AddClientInfo(conn, GetIPAddress(conn->socket), req.info().client().name(), req.info().client().listengroup() | ADMIN_GROUP_MASK);
		Room* added = nullptr;
		auto err = AddRoom(&added, info, req.info().serverid());

		Base::CreateResponse response;
		response.set_error(err);
		response.set_id(info->id, 16);
		response.mutable_info()->mutable_client()->set_name(req.info().client().name());
		response.mutable_info()->set_serverid(req.info().serverid());
		response.mutable_info()->mutable_client()->set_listengroup(info->groupMask);
		std::string serialized = response.SerializeAsString();
		conn->SendData(PacketID::CREATE, serialized.size(), serialized.data());
	}


	UnlockResourceAccess();
}

void ClientConnectCB(void* userData, TCPServerSocket* server, Connection* conn)
{
	std::string clientIP = GetIPAddress(conn->socket);
	LOG("Client IP Addr: %s\n", clientIP.c_str());

}

void ClientDisconnectCB(void* userData, TCPServerSocket* sock, Connection* removed)
{
	LockResourceAccess();
	ClientInfo* removedClient = GetClientInfo(removed);
	if (removedClient)
	{
		LOG("CLIENT DISCONNECTED:\t%s\n", removedClient->name.c_str());
		removedClient->conn = nullptr;
	}
	UnlockResourceAccess();
}


int main()
{
	NetError err = NetError::OK;
	TCPServerSocket server;


	server.SetPacketCallback(PacketCB, &server);
	server.SetConnectCallback(ClientConnectCB);
	server.SetDisconnectCallback(ClientDisconnectCB);

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