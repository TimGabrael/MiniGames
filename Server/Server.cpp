#include <iostream>
#include "CommonCollection.h"
#include "Network/Messages/join.pb.h"
#include "Network/Messages/sync.pb.h"
#include "Room.h"
#include "Validation.h"

void PacketCB(void* userData, Connection* conn, Packet* packet)
{
	LockResourceAccess();

	TCPServerSocket* server = (TCPServerSocket*)userData;
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
			info = GetClientInfo(conn);
			if (!info)
			{
				if (j.has_id() && j.id().size() == 16)
				{
					info = AddClientInfo(conn, GetIPAddress(conn->socket), name, j.info().client().listengroup() & ~(ADMIN_GROUP_MASK), (const uint8_t*)j.id().data());
				}
				else
				{
					info = AddClientInfo(conn, GetIPAddress(conn->socket), name, j.info().client().listengroup() & ~(ADMIN_GROUP_MASK));
				}
			}
			else
			{
				info->name = name;
				info->groupMask = j.info().client().listengroup() & ~(ADMIN_GROUP_MASK);
			}
			Room* room = nullptr;
			err = ClientJoinRoom(&room, info, lobby);
			if (err == Base::SERVER_ROOM_JOIN_INFO::ROOM_JOIN_OK)
			{
				Base::AddClient addMsg;
				addMsg.mutable_joined()->set_name(name);
				addMsg.mutable_joined()->set_listengroup(info->groupMask);
				std::string addSer = addMsg.SerializeAsString();
				if (room) {
					for (int i = 0; i < room->clients.size(); i++)
					{
						auto c = room->clients.at(i);
						if (c != info)
						{
							c->conn->SendData(PacketID::ADD_CLIENT, addSer);
						}
					}
				}
			}
			for (int i = 0; i < j.info().availableplugins_size(); i++)
			{
				LOG("PLUGIN: %s\n", j.info().availableplugins(i).c_str());
			}

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
	else if (packet->header.type == (uint32_t)PacketID::CREATE)
	{
		Base::CreateRequest req;
		req.ParseFromArray(packet->body.data(), packet->body.size());

		for (int i = 0; i < req.info().availableplugins_size(); i++)
		{
			LOG("PLUGIN: %s\n", req.info().availableplugins(i).c_str());
		}

		std::string name = req.info().client().name();
		std::string lobby = req.info().serverid();

		Base::CreateResponse response;
		if (ValidateName(name) && ValidateServerID(lobby))
		{

			ClientInfo* info = AddClientInfo(conn, GetIPAddress(conn->socket), name, req.info().client().listengroup() | ADMIN_GROUP_MASK);
			Room* added = nullptr;
			auto err = AddRoom(&added, info, req.info().serverid());
			if (added)
			{
				for (int i = 0; i < req.info().availableplugins_size(); i++)
				{
					added->activePlugins.push_back(req.info().availableplugins(i));
				}
			}
			added->clients.push_back(info);

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
	else if (packet->header.type == (uint32_t)PacketID::SYNC_REQUEST)
	{
		ClientInfo* client = GetClientInfo(conn);
		if (client && client->room)
		{
			client->room->waitForSyncClients.push_back(client);
			client->room->admin->conn->SendData(PacketID::SYNC_REQUEST, 0, nullptr);
		}
	}
	else if (packet->header.type == (uint32_t)PacketID::SYNC_RESPONSE)
	{
		ClientInfo* client = GetClientInfo(conn);
		if (client && client->room && (client->room->admin == client))
		{
			Room* r = client->room;
			Base::SyncResponse res;
			res.ParseFromArray(packet->body.data(), packet->body.size());

			res.clear_availableplugins();
			res.clear_connectedclients();
			res.clear_serverid();
			
			for (int i = 0; i < r->activePlugins.size(); i++)
			{
				res.add_availableplugins(r->activePlugins.at(i).c_str());
			}
			for (int i = 0; i < r->clients.size(); i++)
			{
				ClientInfo* c = r->clients.at(i);
				Base::ClientInfo* cl = res.add_connectedclients();
				cl->set_name(c->name); cl->set_listengroup(c->groupMask);
			}
			res.set_serverid(r->ID);
			const std::string serialized = res.SerializeAsString();
			for (int i = 0; i < r->waitForSyncClients.size(); i++)
			{
				client->room->waitForSyncClients.at(i)->conn->SendData(PacketID::SYNC_RESPONSE, serialized);
			}
			client->room->waitForSyncClients.clear();
		}
	}
	else
	{
		ClientInfo* client = GetClientInfo(conn);
		if (client->room)
		{
			for (ClientInfo* cl : client->room->clients)
			{
				if (cl == client) continue;
				if (cl->groupMask & packet->header.group)	// check if the message should be send to the client
				{
					cl->conn->SendData((PacketID)packet->header.type, packet->header.group, packet->header.additionalData, packet->body.size(), packet->body.data());
				}
			}
		}
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
		Base::RemoveClient rem;
		rem.mutable_removed()->set_name(removedClient->name);
		rem.mutable_removed()->set_listengroup(removedClient->groupMask);
		const std::string removeData = rem.SerializeAsString();
		LOG("CLIENT DISCONNECTED:\t%s\n", removedClient->name.c_str());
		removedClient->conn = nullptr;
		if (removedClient->room)
		{
			Room* r = removedClient->room;
			for (int i = 0; i < r->clients.size(); i++)
			{
				if (removedClient != r->clients.at(i))
				{
					if (r->clients.at(i)->conn)
					{
						r->clients.at(i)->conn->SendData(PacketID::REMOVE_CLIENT, removeData);
					}
				}
			}
		}
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