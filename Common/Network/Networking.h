#pragma once
#include <stdint.h>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>


enum class NetError
{
	OK,
	E_INIT,		// failed to initialize
	E_RESOLVE,	// failed to Resolve the address
	E_CONNECT,	// failed to Connect
	E_LISTEN,	// failed to Listen
	E_READ,		// failed to Read bytes from socket
};


struct PacketHeader
{
	PacketHeader() { type = 0; size = 0; }
	constexpr PacketHeader(uint32_t t, uint32_t sz) : type(t), size(sz) { }
	uint32_t type;
	uint32_t size;
};
struct Packet
{
	PacketHeader header;
	std::vector<char> body;
};

struct PacketParent
{
	PacketParent(uint32_t type, uint32_t size) : header(type, size) { }
	PacketHeader header;
};

struct TestPacket : public PacketParent
{
	TestPacket(const char* str) : PacketParent(1, sizeof(TestPacket) - sizeof(PacketParent))
	{
		memcpy(buf, str, 100);
		buf[99] = '\00';
	}
	char buf[100];
};


typedef void(*ConnectionFunc)(void* obj, struct Connection* conn);

struct Connection
{
	Connection();
	Connection(uintptr_t sock);
	Connection(uintptr_t sock, ConnectionFunc socketCallback);
	Connection(uintptr_t sock, ConnectionFunc socketCallback, void* obj);
	uintptr_t socket;
	std::thread listener;
	Packet storedPacket;
};

class TCPSocket
{
public:

	NetError Connect(const char* host, const char* port);
	void Disconnect();

	void SendPacket(PacketParent* pack);

private:
	uintptr_t sock;
	Connection serverConn;
};

class TCPServerSocket
{
public:
	NetError Create(const char* host, const char* port);
	NetError AcceptConnection();

	void RemoveAll();

	void Send(uint32_t clientIdx);

	static void TCPServerListenToClient(void* server, Connection* conn);

private:
	void AddClient(uintptr_t connSocket);
	void RemoveClient(uintptr_t connSocket);
	void RemoveClientAtIdx(size_t idx);

	void RemoveInListener(uintptr_t connSocket);

	uintptr_t sock;
	std::mutex clientMut;
	std::vector<Connection> clients;
	bool running;
};