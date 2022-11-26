#pragma once
#include <stdint.h>
#include <vector>



// ADMIN/CREATOR OF THE ROOM
#define ADMIN_GROUP_MASK (1 << 31)
#define STANDARD_GROUP_MASK 1
#define LISTEN_GROUP_ALL 0xFFFFFFFF

#define MIN_NAME_LENGTH 5
#define MAX_NAME_LENGTH 30


typedef int(__stdcall* PacketFunction)(void* packet);

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



enum ServerPacketID : uint16_t
{
	SERVER_PACKET_ACK = 0,
	SERVER_PACKET_JOIN = 1,
	SERVER_PACKET_JOIN_RESPONSE = 2,
	SERVER_PACKET_SYNC_RESPONSE = 3,	// these need to be seperate as the server can ask the Admin for the current SYNC data
	SERVER_PACKET_ADD_CLIENT = 4,
	SERVER_PACKET_REMOVE_CLIENT = 5,
	SERVER_PACKET_FORCE_SYNC = 6,
	SERVER_PACKET_CLIENTS = 7,
	SERVER_PACKET_CLIENT_DISCONNECT = 8,
	// END OF GENERAL MESSAGES


	SERVER_VOTE = 9,
	SERVER_START = 10,


	SERVER_IMPORTANT_FLAG = (1 << 15),
};

enum ClientPacketID : uint16_t
{
	CLIENT_PACKET_ACK = 0,
	CLIENT_PACKET_PING = 1,
	CLIENT_PACKET_JOIN = 2,
	CLIENT_PACKET_SYNC_REQUEST = 3,
	CLIENT_PACKET_DISCONNECT = 4,
	// END OF GENERAL MESSAGES


	CLIENT_VOTE = 5,
	CLIENT_START = 6,


	CLIENT_IMPORTANT_FLAG = (1 << 15),
};


struct BaseHeader
{
	uint16_t packetID;
	uint16_t sequenceNumber;
};

struct ClientJoinPacket
{
	uint16_t packetID;
	uint16_t sequenceNumber;
	char name[MAX_NAME_LENGTH];
};

namespace Client
{


}

namespace Server
{
	struct JoinResponsePacket
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		uint16_t id;
		uint16_t error;
		char name[MAX_NAME_LENGTH];
	};
}



JOIN_ERROR ValidatePlayerName(char name[MAX_NAME_LENGTH]);

