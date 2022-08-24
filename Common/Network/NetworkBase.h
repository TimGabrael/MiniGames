#pragma once
#include <stdint.h>
#include <vector>



// ADMIN/CREATOR OF THE ROOM
#define ADMIN_GROUP_MASK (1 << 31)
#define STANDARD_GROUP_MASK 1
#define LISTEN_GROUP_ALL 0xFFFFFFFF

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

enum class PacketID
{
	HANDSHAKE = 0,
	CHECK = 1,
	JOIN = 2,
	CREATE = 3,
	SYNC_REQUEST = 4,	// these need to be seperate as the server can ask the Admin for the current SYNC data
	SYNC_RESPONSE = 5,	// these need to be seperate as the server can ask the Admin for the current SYNC data
	ADD_CLIENT = 6,
	REMOVE_CLIENT = 7,
	FORCE_SYNC = 8,
	GET_CLIENTS = 9,
	// END OF GENERAL MESSAGES


	VOTE = 10,
	VOTE_SYNC = 11,
	START = 12,
	STARTED = 13,
	NUM_PACKETS = 14,
};

enum AdditionalDataFlags : uint16_t
{
	ADDITIONAL_DATA_FLAG_ADMIN = 1,				// only admins should and can send this message!
	ADDITIONAL_DATA_FLAG_TO_SENDER_ID = 1 << 1,	// send only to senderID
};


struct PacketHeader
{
	PacketHeader() { type = 0; size = 0; group = 0x0; additionalData = 0; senderID = 0; }
	constexpr PacketHeader(uint32_t t, uint32_t sz) : type(t), size(sz), group(0x0), additionalData(0), senderID(0) { }
	constexpr PacketHeader(PacketID t, uint32_t sz) : type((uint32_t)t), size(sz), group(0x0), additionalData(0), senderID(0) { }
	constexpr PacketHeader(PacketID t, uint32_t gr, uint32_t sz) : type((uint32_t)t), size(sz), group(gr), additionalData(0), senderID(0) { }
	constexpr PacketHeader(PacketID t, uint32_t gr, uint16_t id, uint32_t sz) : type((uint32_t)t), size(sz), group(gr), additionalData(0), senderID(id) { }
	constexpr PacketHeader(PacketID t, uint32_t gr, uint16_t additional, uint16_t id, uint32_t sz) : type((uint32_t)t), size(sz), group(gr), additionalData(additional), senderID(id) { }
	constexpr PacketHeader(uint32_t t, uint32_t gr, uint16_t additional, uint16_t id, uint32_t sz) : type(t), size(sz), group(gr), additionalData(additional), senderID(id) { }
	uint32_t type;
	uint32_t size;
	uint32_t group;
	uint16_t additionalData;
	uint16_t senderID;
};
struct Packet
{
	PacketHeader header;
	std::vector<char> body;
};