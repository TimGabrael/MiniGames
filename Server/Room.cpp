#include "Room.h"
#include <random>

static std::mutex resourceMutex;
static std::vector<Room*> rooms;
static std::vector<ClientInfo*> clients;

uint8_t random()
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, 0xFF);
	return dist(rng);
}


ClientInfo* GetClientInfo(Connection* conn)
{
	for (auto& c : clients)
	{
		if (c->conn == conn)
			return c;
	}
	return nullptr;
}
int GetClientIdx(Connection* conn)
{
	for (int i = 0; i < clients.size(); i++)
	{
		auto& c = clients.at(i);
		if (c->conn == conn)
			return i;
	}
	return -1;
}
ClientInfo* AddClientInfo(Connection* conn, const std::string& ipAddr, const std::string& name, uint32_t mask)
{
	int idx = clients.size();
	clients.push_back(new ClientInfo());
	ClientInfo* c = clients.at(idx);
	c->conn = conn; c->ipAddr = ipAddr; c->name = name; c->room = nullptr; c->timeOutTimer = 0.0f; c->groupMask = mask;
	for (int i = 0; i < 16; i++)
	{
		c->id[i] = random();
	}
	return c;
}
ClientInfo* AddClientInfo(Connection* conn, const std::string& ipAddr, const std::string& name, uint32_t mask, const uint8_t id[16])
{
	int idx = -1;
	for (int i = 0; i < clients.size(); i++)
	{
		if (memcmp(clients.at(i)->id, id, 16) == 0)
		{
			idx = i;
			break;
		}
	}
	ClientInfo* c = nullptr;
	if (idx == -1)
	{
		idx = clients.size();
		clients.push_back(new ClientInfo());
		c = clients.at(idx);
		for (int i = 0; i < 16; i++)
		{
			c->id[i] = random();
		}
	}
	c->conn = conn; c->ipAddr = ipAddr;
	c->name = name; c->room = nullptr; c->groupMask = mask;
	return c;
}


Room* GetRoomByID(const std::string& ID)
{
	for (int i = 0; i < rooms.size(); i++)
	{
		if (rooms.at(i)->ID == ID)
			return rooms.at(i);
	}
	return nullptr;
}
Base::SERVER_ROOM_JOIN_INFO ClientJoinRoom(ClientInfo* info, const std::string& ID)
{
	for (int i = 0; i < rooms.size(); i++)
	{
		if (rooms.at(i)->ID == ID) { // ADD THE CLIENT HERR
			return Base::SERVER_ROOM_JOIN_INFO::ROOM_JOIN_OK;
		}
	}


	return Base::SERVER_ROOM_JOIN_INFO::ROOM_JOIN_UNAVAILABLE;
}
Base::SERVER_ROOM_CREATE_INFO AddRoom(Room** room, ClientInfo* client, const std::string& ID)
{
	if (room) *room = nullptr;
	if (client)
	{
		const int roomIdx = rooms.size();
		for (int i = 0; i < roomIdx; i++)
		{
			if (rooms.at(i)->ID == ID) return Base::SERVER_ROOM_CREATE_INFO::ROOM_CREATE_COLLISION;
		}
		rooms.push_back(new Room());
		Room* r = rooms.at(roomIdx); r->admin = client; r->ID = ID;
		if (room) *room = r;
	}
	return Base::SERVER_ROOM_CREATE_INFO::ROOM_CREATE_OK;
}

void LockResourceAccess()
{
	resourceMutex.lock();
}
void UnlockResourceAccess()
{
	resourceMutex.unlock();
}