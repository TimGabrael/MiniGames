#include <iostream>
#include "CommonCollection.h"
#include "Network/Messages/join.pb.h"
#include "Room.h"
#include "Validation.h"

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
		
		std::string name = j.info().client().name();
		std::string lobby = j.info().serverid();

		Base::JoinResponse response;
		if (ValidateName(name) && ValidateServerID(lobby))
		{
			ClientInfo* info = nullptr;
			Base::SERVER_ROOM_JOIN_INFO err = Base::SERVER_ROOM_JOIN_INFO::ROOM_JOIN_OK;
			if (j.has_id() && j.id().size() == 16)
			{
				info = AddClientInfo(conn, GetIPAddress(conn->socket), name, j.info().client().listengroup() & ~(ADMIN_GROUP_MASK), (const uint8_t*)j.id().data());

			}
			else
			{
				info = AddClientInfo(conn, GetIPAddress(conn->socket), name, j.info().client().listengroup() & ~(ADMIN_GROUP_MASK));
			}
			err = ClientJoinRoom(info, lobby);

			LOG("%s, %s\n", name.c_str(), lobby.c_str());
			response.set_error(err);
			response.set_id(info->id, 16);
			response.mutable_info()->mutable_client()->set_name(info->name);
			response.mutable_info()->set_serverid(lobby);
			response.mutable_info()->mutable_client()->set_listengroup(info->groupMask);
		}
		else
		{
			response.set_error(Base::SERVER_ROOM_JOIN_INFO::ROOM_JOIN_INVALID_MESSAGE);
		}
		conn->SendData(PacketID::JOIN, response.SerializeAsString());
	}
	if (packet->header.type == (uint32_t)PacketID::CREATE)
	{
		Base::CreateRequest req;
		req.ParseFromArray(packet->body.data(), packet->body.size());

		std::string name = req.info().client().name();
		std::string lobby = req.info().serverid();

		Base::CreateResponse response;
		if (ValidateName(name) && ValidateServerID(lobby))
		{

			ClientInfo* info = AddClientInfo(conn, GetIPAddress(conn->socket), name, req.info().client().listengroup() | ADMIN_GROUP_MASK);
			Room* added = nullptr;
			auto err = AddRoom(&added, info, req.info().serverid());

			response.set_error(err);
			response.set_id(info->id, 16);
			response.mutable_info()->mutable_client()->set_name(req.info().client().name());
			response.mutable_info()->set_serverid(req.info().serverid());
			response.mutable_info()->mutable_client()->set_listengroup(info->groupMask);
		}
		else
		{
			response.set_error(Base::SERVER_ROOM_CREATE_INFO::ROOM_CREATE_INVALID_MESSAGE);
		}
		conn->SendData(PacketID::CREATE, response.SerializeAsString());
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