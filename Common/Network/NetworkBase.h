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


// should return the size of the packet negative numbers will discard the entire packet
typedef int(__stdcall* ClientPacketFunction)(void* socket, void* packet, int packetSize);
typedef int(__stdcall* ServerPacketFunction)(void* socket, void* client, void* packet, int packetSize);
typedef bool(__stdcall* JoinCallbackFunction)(void* socket, void* client);
typedef void(__stdcall* DisconnectCallbackFunction)(void* socket, void* client);

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
	SERVER_PACKET_ADD_CLIENT = 2,
	SERVER_PACKET_REMOVE_CLIENT = 3,
	SERVER_PACKET_CLIENTS = 4,
	SERVER_PACKET_CLIENT_DISCONNECT = 5,
	SERVER_PACKET_SYNC = 6,
	
	NUM_BASE_SERVER_PACKETS = SERVER_PACKET_SYNC + 1,


	SERVER_IMPORTANT_FLAG = (1 << 15),
};

enum ClientPacketID : uint16_t
{
	CLIENT_PACKET_ACK = 0,
	CLIENT_PACKET_PING = 1,
	CLIENT_PACKET_JOIN = 2,
	CLIENT_PACKET_DISCONNECT = 3,

	NUM_BASE_CLIENT_PACKETS = CLIENT_PACKET_DISCONNECT + 1,

	CLIENT_IMPORTANT_FLAG = (1 << 15),
};


struct BaseHeader
{
	uint16_t packetID;
	uint16_t sequenceNumber;
};



namespace Client
{
	enum LobbyPacketID
	{
		CLIENT_VOTE = NUM_BASE_CLIENT_PACKETS,
		Client_START,
	};

	struct JoinPacket
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		char name[MAX_NAME_LENGTH];
	};

	struct VotePacket
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		uint16_t voteID;
	};

	struct VoteStartPacket
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		float timer;
	};

}

namespace Server
{
	enum LobbyPacketID
	{
		SERVER_START_PLUGIN = NUM_BASE_SERVER_PACKETS,
		SERVER_VOTE_DATA,
		SERVER_AVAILABLE_PLUGIN,
	};
	struct JoinResponsePacket
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		uint16_t id;
		uint16_t error;
		char name[MAX_NAME_LENGTH];
		bool isAdmin;
	};

	struct AddClient
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		uint16_t clientID;
		char name[MAX_NAME_LENGTH];
		bool isAdmin;
	};
	struct RemoveClient
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		uint16_t clientID;
	};
	struct Client
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		uint16_t clientID;
		char name[MAX_NAME_LENGTH];
		bool isAdmin;
	};
	struct Disconnect
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		uint16_t reason;
	};
	struct StartPlugin
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		char plugin[20];
	};
	struct VotePacket
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		uint16_t clientID;
		uint16_t votedPluginID;
	};
	struct PluginData
	{
		uint16_t packetID;
		uint16_t sequenceNumber;
		uint16_t pluginSessionID;
		char pluginID[19];
	};

}



JOIN_ERROR ValidatePlayerName(const char name[MAX_NAME_LENGTH]);
JOIN_ERROR ValidatePlayerName(const std::string& name);

