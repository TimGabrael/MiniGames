#include "Networking.h"
#include <random>
#include "../logging.h"

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
static constexpr int INVALID_SOCKET = ~(0LL);
typedef int SOCKET;
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#ifndef _WIN32
static void closesocket(uintptr_t socket) { close(socket); }
#endif


//#define PRINT_CRYPTO_KEYS
void PrintKey(const ukey_t& k)
{
	static constexpr uint32_t num64 = sizeof(ukey_t) / 8;
	const uint64_t* k64 = (uint64_t*)k.byte;
	for (uint32_t i = 0; i < num64; i++)
	{
		LOG("%016llX ", k64[i]);
	}
	LOG("\n");
}





bool SendDataToSocket(SOCKET sock, PacketID id, uint32_t size, const uint8_t* data)
{
	if (sock == INVALID_SOCKET) return false;

	// send header
	{
		PacketHeader head(id, size);
		uint32_t sendBytes = 0;
		while (sendBytes < sizeof(PacketHeader))
		{
			uint32_t temp = send(sock, ((const char*)&head) + sendBytes, sizeof(PacketHeader) - sendBytes, 0);
			if (temp == -1) return false;
			sendBytes += temp;
		}
	}
	// send body
	{
		uint32_t sendBytes = 0;
		while (size > sendBytes)
		{
			uint32_t temp = send(sock, (const char*)data + sendBytes, size - sendBytes, 0);
			if (temp == -1) return false;
			sendBytes += temp;
		}
	}
	return true;
}
bool SendDataToSocket(SOCKET sock, const PacketHeader& header, const uint8_t* data, uint32_t dataSize)
{
	if (sock == INVALID_SOCKET) return false;
	{
		uint32_t sendBytes = 0;
		while (sendBytes < sizeof(PacketHeader))
		{
			uint32_t temp = send(sock, ((const char*)&header) + sendBytes, sizeof(PacketHeader) - sendBytes, 0);
			if (temp == -1) return false;
			sendBytes += temp;
		}
	}
	{
		uint32_t sendBytes = 0;
		while (dataSize > sendBytes)
		{
			uint32_t temp = send(sock, (const char*)data + sendBytes, dataSize - sendBytes, 0);
			if (temp == -1) return false;
			sendBytes += temp;
		}
	}
	return true;
}










uint8_t GenRandomNumber()
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, 0xFF);
	return dist(rng);
}

ValidationPacket CreateValidationPacket()
{
	ValidationPacket pack;
	for (int i = 0; i < 64; i++)
	{
		pack.PacketData[i] = GenRandomNumber();
	}
	return pack;
}
// Is used to check if the server and the application are of the same version
void TransformValidationPacket(ValidationPacket& pack)
{
	uint8_t* d = pack.PacketData; 
	d[0] += 10; d[1] = d[9] * 3; d[2] ^= d[3]; d[3] = d[7]; d[10] += 10 * d[15]; d[42] = 34 + d[50]; d[5] = d[62] + 2;
	d[47] ^= d[8];
	uint32_t* d1 = (uint32_t*)d;
	d1[4] = d1[2] * 45;
	d1[8] = d1[1] * 10;
}




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
		LOG("Failed To Create Socket code: %d\n", err);
		CleanUp();
		return NetError::E_CONNECT;
	}
#else
	if (sock == INVALID_SOCKET)
	{
		LOG("Failed To Create Socket\n");
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
			LOG("[ERROR] failed to read header\n");
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
		int cur = recv(sock, fill->body.data() + readBytes, fill->header.size, 0);
		if (cur <= 0)
		{
			LOG("[ERROR] failed to read body\n");
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
Connection::Connection(uintptr_t sock, ConnectionFunc socketCallback, void* obj, const ukey_t& key)
{
	Init(sock, socketCallback, obj, key);
}
Connection::~Connection()
{
	if(socket != INVALID_SOCKET) closesocket(socket);
	socket = INVALID_SOCKET;
	if (listener.get_id() == std::this_thread::get_id())
	{
		listener.detach();
	}
	else
	{
		if (listener.joinable()) listener.join();
	}
}
void Connection::Init(uintptr_t sock, ConnectionFunc socketCallback, void* obj, const ukey_t& key)
{
	socket = sock; SetCryptoKey(key);
	listener = std::thread(socketCallback, obj, this);
}
void Connection::SendData(PacketID id, size_t size, const void* data)
{
	uint8_t* packData = new uint8_t[size];
	memcpy(packData, data, size);
	PacketHeader pack(id, size);
	ctx.EncryptBuffer(packData, size);
	ctx.EncryptBuffer((uint8_t*)&pack, sizeof(PacketHeader));
	SendDataToSocket(socket, pack, packData, size);

	delete[] packData;
}
void Connection::SetCryptoKey(const ukey_t& k)
{
	this->ctx.InitContext(k.byte);
}
TCPSocket::TCPSocket()
{
	GenKeyPair(&publicKey, &privateKey);
}
NetError TCPSocket::Connect(const char* host, const char* port)
{
	addrinfo* info = nullptr;
	NetError err = BuildTcpSocket(host, port, sock, &info);
	if (err != NetError::OK) return err;

	int res = connect(sock, info->ai_addr, (int)info->ai_addrlen);

	if (res != 0)
	{
		LOG("Connection Failed ErrorCode: %d\n", errno);
		closesocket(sock);
		sock = INVALID_SOCKET;
		CleanUp();
		return NetError::E_CONNECT;
	}


	// PERFORM HANDSHAKE AND VALIDATION HERE
	ukey_t sharedSecret;
	Packet pack;
	err = ReadHeader(sock, &pack);
	if (err != NetError::OK) return NetError::E_HANDSHAKE;
	if (pack.header.type != (uint32_t)PacketID::HANDSHAKE || pack.header.size != sizeof(ukey_t)) return NetError::E_HANDSHAKE;

	err = ReadBody(sock, &pack);
	if (err != NetError::OK) return NetError::E_HANDSHAKE;

	SendDataUnencrypted(PacketID::HANDSHAKE, sizeof(sharedSecret), publicKey.byte);
	ukey_t& pubKeyServer = *(ukey_t*)pack.body.data();

	GenSharedSecret(&sharedSecret, privateKey, pubKeyServer);
	CryptoContext tempCtx;
	tempCtx.InitContext(sharedSecret.byte);


	err = ReadHeader(sock, &pack);
	if (err != NetError::OK) return NetError::E_READ;
	if (pack.header.type != (uint32_t)PacketID::CHECK || pack.header.size != sizeof(ValidationPacket)) return NetError::E_VERSION;

	err = ReadBody(sock, &pack);
	if (err != NetError::OK) return NetError::E_READ;
	
	ValidationPacket* valPack = (ValidationPacket*)pack.body.data();
	tempCtx.DecryptBuffer(valPack->PacketData, sizeof(ValidationPacket));
	TransformValidationPacket(*valPack);

	tempCtx.EncryptBuffer(valPack->PacketData, sizeof(ValidationPacket));
	SendDataUnencrypted(PacketID::CHECK, sizeof(ValidationPacket), valPack->PacketData);

	serverConn.Init(sock, TCPSocket::SocketListenFunc, this, sharedSecret);


#ifdef PRINT_CRYPTO_KEYS
	LOG("PUBLIC_KEY:\n");
	PrintKey(publicKey);
	LOG("RECV_PUBLIC_KEY:\n");
	PrintKey(pubKeyServer);
	LOG("PRIVATE_KEY:\n");
	PrintKey(privateKey);
	LOG("SHARED_SECRET:\n");
	PrintKey(sharedSecret);
#endif




	return NetError::OK;
}
void TCPSocket::Disconnect()
{
	if (sock == INVALID_SOCKET) return;
	closesocket(sock);
	sock = INVALID_SOCKET;
	this->running = false;
	if (serverConn.listener.joinable()) serverConn.listener.join();
}
void TCPSocket::SetDisconnectCallback(ClientDisconnectCallback cb, void* userData)
{
	this->disconnectCB = cb; this->userDataDisconnectCB = userData;
}
void TCPSocket::SetPacketCallback(ClientPacketCallback cb, void* userData)
{
	this->packCb = cb; this->userDataPacketCB = userData;
}
void TCPSocket::SendData(PacketID id, uint32_t size, const uint8_t* data)
{
	this->serverConn.SendData(id, size, (const void*)data);
}
void TCPSocket::SendDataUnencrypted(PacketID id, uint32_t size, const uint8_t* data)
{
	SendDataToSocket(sock, id, size, data);
}
void TCPSocket::SocketListenFunc(void* client, struct Connection* conn)
{
	TCPSocket* s = (TCPSocket*)client;
	Packet* cur = &conn->storedPacket;
	while (s->running)
	{
		NetError err = ReadHeader(conn->socket, cur);
		if (err != NetError::OK)
		{
			if (s->disconnectCB) s->disconnectCB(s->userDataDisconnectCB, s, &s->serverConn);
			s->running = false; return;
		}
		conn->ctx.DecryptBuffer((uint8_t*)&cur->header, sizeof(PacketHeader));

		err = ReadBody(conn->socket, cur);
		if (err != NetError::OK)
		{
			if (s->disconnectCB) s->disconnectCB(s->userDataDisconnectCB, s, &s->serverConn);
			s->running = false; return;
		}
		// the packet is finished at this point.
		conn->ctx.DecryptBuffer((uint8_t*)cur->body.data(), cur->header.size);
		if (s->packCb)
			s->packCb(s->userDataPacketCB, cur);
	}
	if (s->disconnectCB) s->disconnectCB(s->userDataDisconnectCB, s, &s->serverConn);
}


TCPServerSocket::TCPServerSocket()
{
	GenKeyPair(&publicKey, &privateKey);
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
			s->RemoveClient(conn->socket);
			return;
		}
		conn->ctx.DecryptBuffer((uint8_t*)&cur->header, sizeof(PacketHeader));
		err = ReadBody(conn->socket, cur);
		if (err != NetError::OK)
		{
			s->RemoveClient(conn->socket);
			return;
		}
		// the packet is finished at this point.
		conn->ctx.DecryptBuffer((uint8_t*)cur->body.data(), cur->header.size);
		if (s->packetCallback)
			s->packetCallback(s->userDataPacketCB, conn, cur);
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
	if (conn != INVALID_SOCKET)
	{
		// PERFORM HANDSHAKE AND VALIDATION HERE
		ukey_t sharedSecret;
		Packet pack;
		SendDataToSocket(conn, PacketID::HANDSHAKE, sizeof(ukey_t), publicKey.byte);
		NetError err = ReadHeader(conn, &pack);
		if (err != NetError::OK) { closesocket(conn); return NetError::E_HANDSHAKE; }
		if (pack.header.type != (uint32_t)PacketID::HANDSHAKE || pack.header.size != sizeof(ukey_t)) { closesocket(conn); return NetError::E_HANDSHAKE; }

		err = ReadBody(conn, &pack);
		if (err != NetError::OK) { closesocket(conn); return NetError::E_HANDSHAKE; }
		ukey_t& pubKeyClient = *(ukey_t*)pack.body.data();

		GenSharedSecret(&sharedSecret, privateKey, pubKeyClient);

		CryptoContext tempCtx;
		tempCtx.InitContext(sharedSecret.byte);
		ValidationPacket valPack = CreateValidationPacket();
		tempCtx.EncryptBuffer(valPack.PacketData, sizeof(ValidationPacket));

		SendDataToSocket(conn, PacketID::CHECK, sizeof(ValidationPacket), valPack.PacketData);

		tempCtx.DecryptBuffer(valPack.PacketData, sizeof(ValidationPacket));
		TransformValidationPacket(valPack);

		err = ReadHeader(conn, &pack);
		if (err != NetError::OK) { closesocket(conn); return NetError::E_READ; }
		if (pack.header.type != (uint32_t)PacketID::CHECK || pack.header.size != sizeof(ValidationPacket)) { closesocket(conn); return NetError::E_VERSION; }

		err = ReadBody(conn, &pack);
		if (err != NetError::OK) { closesocket(conn); return NetError::E_READ; }
		ValidationPacket* recValPack = (ValidationPacket*)pack.body.data();
		tempCtx.DecryptBuffer(recValPack->PacketData, sizeof(ValidationPacket));

		for (int i = 0; i < 64; i++)
		{
			if (recValPack->PacketData[i] != valPack.PacketData[i])
			{
				LOG("CLIENT_VERSION_MISMATCH\n");
				closesocket(conn);
				return NetError::E_VERSION;
			}
		}
		







#ifdef PRINT_CRYPTO_KEYS
		LOG("PUBLIC_KEY:\n");
		PrintKey(publicKey);
		LOG("RECV_PUBLIC_KEY:\n");
		PrintKey(pubKeyClient);
		LOG("PRIVATE_KEY:\n");
		PrintKey(privateKey);
		LOG("SHARED_SECRET:\n");
		PrintKey(sharedSecret);
#endif

		


		AddClient(conn, sharedSecret);
	}
	return NetError::OK;
}
void TCPServerSocket::SetConnectCallback(ConnectCallback cb, void* userData)
{
	this->connectCallback = cb;
	this->userDataConnectCB = userData;
}
void TCPServerSocket::SetPacketCallback(ServerPacketCallback cb, void* data)
{
	this->packetCallback = cb;
	this->userDataPacketCB = data;
}
void TCPServerSocket::SetDisconnectCallback(DisconnectCallback cb, void* userData)
{
	this->disconnectCallback = cb;
	this->userDataDisconnectCB = userData;
}
void TCPServerSocket::AddClient(uintptr_t connSocket, const ukey_t& key)
{
	clientMut.lock();
	const uint32_t clientIdx = clients.size();
	clients.emplace_back(new Connection(connSocket, TCPServerListenToClient, this, key));
	Connection* conn = clients.at(clientIdx);
	clientMut.unlock();

	if (this->connectCallback)
		this->connectCallback(this->userDataConnectCB, this, conn);
}
void TCPServerSocket::RemoveClient(uintptr_t connSocket)
{
	clientMut.lock();
	for (size_t i = 0; i < clients.size(); i++)
	{
		auto& client = clients.at(i);
		if (client->socket == connSocket)
		{
			if (this->disconnectCallback)
				this->disconnectCallback(this->userDataDisconnectCB, this, client);

			delete clients.at(i);
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
		if (this->disconnectCallback)
			this->disconnectCallback(this->userDataDisconnectCB, this, client);

		delete clients.at(idx);
		clients.erase(clients.begin() + idx);
	}
	clientMut.unlock();
}

void TCPServerSocket::RemoveAll()
{
	while (!clients.empty()) {
		RemoveClientAtIdx(0);
	}
}
void TCPServerSocket::RemoveClient(Connection* conn)
{
	clientMut.lock();
	for (size_t i = 0; i < clients.size(); i++)
	{
		auto* client = clients.at(i);
		if (client == conn)
		{
			if (this->disconnectCallback)
				this->disconnectCallback(this->userDataDisconnectCB, this, client);
			delete client;
			clients.erase(clients.begin() + i);
		}
	}
	clientMut.unlock();
}


std::string GetIPAddress(uintptr_t socket)
{
	sockaddr_in addrInfo; int nameLen;
	getsockname(socket, (sockaddr*)&addrInfo, &nameLen);
	char* ipAddr = inet_ntoa(addrInfo.sin_addr);
	std::string res = ipAddr;
	return res;
}