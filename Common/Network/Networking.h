#pragma once
#include <stdint.h>
#include <thread>
#include <vector>
#include <mutex>
#include "Cryptography.h"

enum class NetError
{
	OK,
	E_INIT,		// failed to initialize
	E_RESOLVE,	// failed to Resolve the address
	E_CONNECT,	// failed to Connect
	E_LISTEN,	// failed to Listen
	E_READ,		// failed to Read bytes from socket
};

enum class PacketID
{
	HANDSHAKE = 0,
	CHECK = 1,

};

struct PacketHeader
{
	PacketHeader() { type = 0; size = 0; }
	constexpr PacketHeader(uint32_t t, uint32_t sz) : type(t), size(sz) { }
	constexpr PacketHeader(PacketID t, uint32_t sz) : type((uint32_t)t), size(sz) { }
	uint32_t type;
	uint32_t size;
};
struct Packet
{
	PacketHeader header;
	std::vector<char> body;
};




typedef void(*ConnectionFunc)(void* obj, struct Connection* conn);
typedef void(*PacketCallback)(void* userData, Packet* pack);

struct Connection
{
	Connection();
	Connection(uintptr_t sock);
	Connection(uintptr_t sock, ConnectionFunc socketCallback);
	Connection(uintptr_t sock, ConnectionFunc socketCallback, void* obj);


	uintptr_t socket;
	std::thread listener;
	Packet storedPacket;
	uint256_t sharedSecret;
};

class TCPSocket
{
public:
	TCPSocket();
	NetError Connect(const char* host, const char* port);
	void Disconnect();

	void SendData(PacketID id, uint32_t size, const uint8_t* data);

private:
	uintptr_t sock;
	Connection serverConn;
	uint256_t privateKey;
	uint256_t publicKey;
};

class TCPServerSocket
{
public:
	TCPServerSocket();
	NetError Create(const char* host, const char* port);
	NetError AcceptConnection();

	void SetPacketCallback(PacketCallback cb, void* userData = nullptr);

	void RemoveAll();

	void Send(uint32_t clientIdx);

	static void TCPServerListenToClient(void* server, Connection* conn);

private:
	void AddClient(uintptr_t connSocket);
	void RemoveClient(uintptr_t connSocket);
	void RemoveClientAtIdx(size_t idx);

	void RemoveInListener(uintptr_t connSocket);

	PacketCallback packetCallback;
	void* userData;
	uintptr_t sock;
	std::mutex clientMut;
	std::vector<Connection> clients;
	uint256_t privateKey;
	uint256_t publicKey;
	bool running;
};

