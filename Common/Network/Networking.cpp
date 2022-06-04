#include "Networking.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#define INVALID_SOCKET -1LL
typedef int SOCKET;
#endif
#include "../logging.h"

#ifndef _WIN32
static void closesocket(uintptr_t socket) { close(socket); }
#endif

void CleanUp()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

NetError BuildTcpSocket(const char* host, const char* port, uintptr_t& sock, addrinfo** info)
{
	sock = INVALID_SOCKET;
	int res = 0;
#ifdef _WIN32
	WSADATA wsaData;
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != 0) {
		return NetError::E_INIT;
	}
#endif

	addrinfo hints{ 0 };
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	res = getaddrinfo(host, port, &hints, info);
	if (res != 0)
	{
		CleanUp();
		return NetError::E_RESOLVE;
	}

	sock = socket((*info)->ai_family, (*info)->ai_socktype, (*info)->ai_protocol);
#ifndef _WIN32
	int err = errno;
	if (err != 0)
	{
		LOG("Failed To Create Socket code: %d", err);
		CleanUp();
		return NetError::E_CONNECT;
	}
#else
	if (sock == INVALID_SOCKET)
	{
		LOG("Failed To Create Socket");
		CleanUp();
		return NetError::E_CONNECT;
	}
#endif
	return NetError::OK;
}

NetError ReadHeader(uintptr_t sock, Packet* fill)
{
	int readBytes = 0;
	while (readBytes < sizeof(PacketHeader))
	{
		int cur = recv(sock, (char*)&fill->header + readBytes, sizeof(PacketHeader), 0);
		if (cur <= 0)
		{
			LOG("[ERROR] failed to read header");
			return NetError::E_READ;
		}
		readBytes += cur;
	}
	return NetError::OK;
}
// fill needs to have a valid header at this point
NetError ReadBody(uintptr_t sock, Packet* fill)
{
	int readBytes = 0;
	fill->body.resize(fill->header.size + sizeof(PacketHeader));
	memcpy(fill->body.data(), &fill->header, sizeof(PacketHeader));

	while (readBytes < fill->header.size)
	{
		int cur = recv(sock, fill->body.data() + readBytes + sizeof(PacketHeader), fill->header.size, 0);
		if (cur <= 0)
		{
			LOG("[ERROR] failed to read body");
			return NetError::E_READ;
		}
		readBytes += cur;
	}
	return NetError::OK;
}

Connection::Connection()
{
	socket = INVALID_SOCKET;
}
Connection::Connection(uintptr_t sock)
{
	socket = sock;
}
Connection::Connection(uintptr_t sock, ConnectionFunc socketCallback)
{
	socket = sock;
	listener = std::thread(socketCallback, nullptr, this);
}
Connection::Connection(uintptr_t sock, ConnectionFunc socketCallback, void* obj)
{
	socket = sock;
	listener = std::thread(socketCallback, obj, this);
}

NetError TCPSocket::Connect(const char* host, const char* port)
{
	addrinfo* info = nullptr;
	NetError err = BuildTcpSocket(host, port, sock, &info);
	if (err != NetError::OK) return err;


	int res = connect(sock, info->ai_addr, (int)info->ai_addrlen);

	if (res != 0)
	{
		LOG("Connection Failed ErrorCode: %d", errno);
		closesocket(sock);

		sock = INVALID_SOCKET;
		CleanUp();
		return NetError::E_CONNECT;
	}

	return NetError::OK;
}
void TCPSocket::Disconnect()
{
	if (sock == INVALID_SOCKET) return;
	closesocket(sock);	
}
void TCPSocket::SendPacket(PacketParent* pack)
{
	uint32_t size = sizeof(PacketHeader) + pack->header.size;
	send(sock, (const char*)pack, size, 0);
}


void TCPServerSocket::TCPServerListenToClient(void* server, Connection* conn)
{
	TCPServerSocket* s = (TCPServerSocket*)server;
	Packet* cur = &conn->storedPacket;
	while (s->running)
	{
		NetError err = ReadHeader(conn->socket, cur);
		if (err != NetError::OK)
		{
			s->RemoveInListener(conn->socket);
			return;
		}
		err = ReadBody(conn->socket, cur);
		if (err != NetError::OK)
		{
			s->RemoveInListener(conn->socket);
			return;
		}
		// the packet is finished at this point.
		TestPacket* pack = (TestPacket*)cur->body.data();
		std::cout << "pack: " << pack->buf << std::endl;

	}
}
NetError TCPServerSocket::Create(const char* host, const char* port)
{
	addrinfo* info = nullptr;
	NetError err = BuildTcpSocket(host, port, sock, &info);
	if (err != NetError::OK) return err;

	int res = bind(sock, info->ai_addr, (int)info->ai_addrlen);
	if (res != 0)
	{
		closesocket(sock);
		sock = INVALID_SOCKET;
		CleanUp();
		return NetError::E_CONNECT;
	}

	running = true;

	return NetError::OK;
}
NetError TCPServerSocket::AcceptConnection()
{
	if (listen(sock, 1) == -1)
	{
		return NetError::E_LISTEN;
	}

	SOCKET conn = accept(sock, nullptr, nullptr);
	if(conn != INVALID_SOCKET)
		AddClient(conn);

	return NetError::OK;
}
void TCPServerSocket::AddClient(uintptr_t connSocket)
{
	clientMut.lock();
	clients.emplace_back(connSocket, TCPServerListenToClient, this);
	clientMut.unlock();
}
void TCPServerSocket::RemoveClient(uintptr_t connSocket)
{
	clientMut.lock();
	for (size_t i = 0; i < clients.size(); i++)
	{
		auto& client = clients.at(i);
		if (client.socket == connSocket)
		{
			closesocket(client.socket);
			if (client.listener.joinable()) client.listener.join();
			
			clients.erase(clients.begin() + i);
			break;
		}
	}
	clientMut.unlock();
}
void TCPServerSocket::RemoveClientAtIdx(size_t idx)
{
	clientMut.lock();
	if (idx < clients.size())
	{
		auto& client = clients.at(idx);
		
		closesocket(client.socket);
		if (client.listener.joinable()) client.listener.join();

		clients.erase(clients.begin() + idx);
	}
	clientMut.unlock();
}

void TCPServerSocket::RemoveInListener(uintptr_t connSocket)
{
	clientMut.lock();
	for (size_t i = 0; i < clients.size(); i++)
	{
		auto& client = clients.at(i);
		if (client.socket == connSocket)
		{
			closesocket(client.socket);
			client.listener.detach();	// needs to be detached, as the destructor trys to join the thread
			clients.erase(clients.begin() + i);
			break;
		}
	}
	clientMut.unlock();
}

void TCPServerSocket::RemoveAll()
{
	while (!clients.empty()) {
		RemoveClientAtIdx(0);
	}
}
void TCPServerSocket::Send(uint32_t clientIdx)
{
	clientMut.lock();
	if (clientIdx < clients.size())
	{
		auto& c = clients.at(clientIdx);
		
	}
	clientMut.unlock();
}