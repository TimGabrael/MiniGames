#pragma once
#include <stdint.h>
#include <vector>
#include <string>


// ADMIN/CREATOR OF THE ROOM
#define ADMIN_GROUP_MASK (1 << 31)
#define STANDARD_GROUP_MASK 1
#define LISTEN_GROUP_ALL 0xFFFFFFFF

#define MIN_NAME_LENGTH 5
#define MAX_NAME_LENGTH 30


struct ClientInfo
{
	std::string name;
	uint16_t id;
};




// should return the size of the packet negative numbers will discard the entire packet
typedef int(__stdcall* ClientPacketFunction)(void* socket, void* packet, int packetSize);
typedef int(__stdcall* ServerPacketFunction)(void* socket, void* client, void* packet, int packetSize);
typedef bool(__stdcall* JoinCallbackFunction)(void* socket, ClientInfo* client);
typedef void(__stdcall* DisconnectCallbackFunction)(void* socket, ClientInfo* client);

enum class NetError
{
	OK,
	E_INIT,			// failed to initialize
	E_RESOLVE,		// failed to Resolve the address
	E_CONNECT,		// failed to Connect
	E_LISTEN,		// failed to Listen
	E_READ,			// failed to Read bytes from socket
	E_HANDSHAKE,	// failed to perform handshake
	E_VERSION,		// failed Version mismatch
};
enum class JOIN_ERROR : uint16_t
{
	JOIN_OK,
	JOIN_NAME_COLLISION,
	JOIN_NAME_INVALID_CHARACTER,
	JOIN_NAME_SHORT,
	JOIN_FULL,
};

