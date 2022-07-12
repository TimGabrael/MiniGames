#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include "Network/Messages/join.pb.h"
#include "Network/Networking.h"

#define INVALID_SOCKET_ID 0xFFFFFFFF



struct Room;
struct ClientInfo
{
	std::string name;
	Connection* conn = nullptr;
	uint8_t id[16];
	Room* room = nullptr;
	std::string ipAddr;
	uint32_t groupMask;
	float timeOutTimer = 0.0f;
};

struct Room
{
	ClientInfo* admin;
	std::vector<ClientInfo*> clients;
	std::vector<ClientInfo*> waitForSyncClients;
	std::vector<std::string> activePlugins;
	std::string ID;
};

ClientInfo* GetClientInfo(Connection* conn);
int GetClientIdx(Connection* conn);
ClientInfo* AddClientInfo(Connection* conn, const std::string& ipAddr, const std::string& name, uint32_t groupMask);
ClientInfo* AddClientInfo(Connection* conn, const std::string& ipAddr, const std::string& name, uint32_t groupMask, const uint8_t id[16]);



Room* GetRoomByID(const std::string& ID);
Base::SERVER_ROOM_JOIN_INFO ClientJoinRoom(Room** r, ClientInfo* info, const std::string& ID);
Base::SERVER_ROOM_CREATE_INFO AddRoom(Room** added, ClientInfo* info, const std::string& ID);



void LockResourceAccess();
void UnlockResourceAccess();