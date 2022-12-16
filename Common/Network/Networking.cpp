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
#include <fcntl.h>
#ifndef _WIN32
static void closesocket(uintptr_t socket) { close(socket); }
#endif


//#define PRINT_CRYPTO_KEYS
static void PrintKey(const ukey_t& k)
{
	static constexpr uint32_t num64 = sizeof(ukey_t) / 8;
	const uint64_t* k64 = (uint64_t*)k.byte;
	for (uint32_t i = 0; i < num64; i++)
	{
		LOG("%016llX ", k64[i]);
	}
	LOG("\n");
}
static uint8_t GenRandomNumber()
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, 0xFF);
	return dist(rng);
}


static bool StartUp()
{
#ifdef _WIN32
	static bool isInitialized = false;
	if (!isInitialized)
	{
		WSADATA wsaData;
		int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (res == 0)
		{
			isInitialized = true;
		}
		return res == 0;
	}
#endif
	return true;
}
static void CleanUp()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

bool SetSocketBlockingEnabled(SOCKET s, bool blocking)
{
	if (s == INVALID_SOCKET) return false;
#ifdef _WIN32
	unsigned long mode = blocking ? 0 : 1;
	return (ioctlsocket(s, FIONBIO, &mode) == 0) ? true : false;
#else
	int flags = fcntl(s, F_GETFL, 0);
	if (flags == -1) return false;
	flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
	return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
#endif
}










struct NetworkingSettings
{
	float resendDelay;
	float pingInterval;
	float timeoutDuration;
};

static constexpr NetworkingSettings settings{
	200.0f,
	1000.0f,
	6000.0f,
};






UDPSocket::UDPSocket()
{
	userData = nullptr;
	sock = INVALID_SOCKET;
	sequenceNumber = 0;
	memset(serverAddr, 0, 16);
	msgBuffer = new char[MAX_UDP_PACKET_SIZE];
	memset(msgBuffer, 0, MAX_UDP_PACKET_SIZE);
}

UDPSocket::~UDPSocket()
{
	delete[] msgBuffer;
}

NetError UDPSocket::Create(const char* host, uint16_t port)
{
	if (sock == INVALID_SOCKET)
	{
		if (!StartUp()) return NetError::E_INIT;
		sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (sock == INVALID_SOCKET) return NetError::E_INIT;
		SetSocketBlockingEnabled(sock, false);
	}
	sockaddr_in* addr = (sockaddr_in*)serverAddr;
	addr->sin_family = AF_INET;
	addr->sin_addr.S_un.S_addr = inet_addr(host);
	addr->sin_port = port;

	return NetError::OK;
}
NetError UDPSocket::Connect(const char* host, uint16_t port, const std::string& name)
{
	if (name.size() > MAX_NAME_LENGTH) return NetError::E_INIT;
	NetError err = Create(host, port);
	if (err != NetError::OK) return err;

	Client::JoinPacket pack{0};
	pack.packetID = CLIENT_PACKET_JOIN | CLIENT_IMPORTANT_FLAG;
	pack.sequenceNumber = sequenceNumber;
	memcpy(pack.name, name.c_str(), name.size());
	SendImportantData(&pack, sizeof(Client::JoinPacket));
	return NetError::OK;
}

bool UDPSocket::SendData(void* data, int size)
{
	sockaddr_in* in = (sockaddr_in*)serverAddr;
	int numBytesWritten = sendto(sock, (const char*)data, size, 0, (sockaddr*)&serverAddr, SOCKADDR_IN_SIZE);
	if (numBytesWritten != size)
	{
		
		LOG("[ERROR]: FAILED TO SEND DATA %d\n", numBytesWritten);
		return false;
	}
	sequenceNumber++;
	return true;
}

bool UDPSocket::SendImportantData(void* data, int size)
{
	if (SendData(data, size))
	{
		char* fullMsg = new char[size];
		memcpy(fullMsg, data, size);
		tempStorage.push_back({ fullMsg, size, 0.0f, 0 });
		return true;
	}
	return false;
}

bool UDPSocket::Poll(float dt)
{
	if (sock == INVALID_SOCKET) return false;
	sockaddr_in server = *(sockaddr_in*)serverAddr;
	if (server.sin_port == 0 || server.sin_family == 0) return false;

	int addrSize = SOCKADDR_IN_SIZE;
	int out = recvfrom(sock, msgBuffer, MAX_UDP_PACKET_SIZE, 0, (sockaddr*)&server, &addrSize);
	if (out > MAX_UDP_PACKET_SIZE || out < 0) return false;
	if (out >= sizeof(BaseHeader))
	{
		BaseHeader* headerInfo = (BaseHeader*)msgBuffer;
		uint16_t id = headerInfo->packetID & ~SERVER_IMPORTANT_FLAG;
		if (headerInfo->packetID & SERVER_IMPORTANT_FLAG)
		{
			SendAck(headerInfo->sequenceNumber);
		}
		if (id == SERVER_PACKET_ACK)
		{
			if (out == sizeof(BaseHeader))
			{
				ReceiveAck(headerInfo->sequenceNumber);
			}
		}
		else if (id < packetHandlers.size() && packetHandlers.at(id))
		{
			int msgLen = packetHandlers.at(id)(this, msgBuffer, out);
		}

	}

	for (int i = 0; i < tempStorage.size(); i++)
	{
		ResendPacketData& resend = tempStorage.at(i);
		resend.accumulatedTime += dt * 100.0f;
		if (resend.accumulatedTime >= settings.resendDelay)
		{
			resend.accumulatedTime = 0.0f;
			SendData(resend.data, resend.size);
		}
	}

	return true;
}
uint16_t UDPSocket::GetSequenceNumber() const
{
	return sequenceNumber;
}

void UDPSocket::SetUserData(void* data)
{
	userData = data;
}

void* UDPSocket::GetUserData()
{
	return userData;
}

void UDPSocket::AddPacketFunction(ClientPacketFunction fun, uint16_t packetID)
{
	if (packetHandlers.size() <= packetID)
	{
		packetHandlers.resize(packetID + 1);
	}
	packetHandlers.at(packetID) = fun;
}

void UDPSocket::RemovePacketFunction(uint16_t packetID)
{
	AddPacketFunction(nullptr, packetID);
}

bool UDPSocket::SendAck(uint16_t sequence) const
{
	BaseHeader ack;
	ack.packetID = CLIENT_PACKET_ACK;
	ack.sequenceNumber = sequence;
	int len = sendto(sock, (const char*)&ack, sizeof(ack), 0, (sockaddr*)serverAddr, SOCKADDR_IN_SIZE);
	return len == sizeof(BaseHeader);
}
void UDPSocket::ReceiveAck(uint16_t sequence)
{
	for (int i = 0; i < tempStorage.size(); i++)
	{
		if (tempStorage.at(i).knownSequenceNumber == sequence)
		{
			delete[] tempStorage.at(i).data;
			tempStorage.erase(tempStorage.begin() + i);
			i--;
		}
	}
}











UDPServerSocket::UDPServerSocket()
{
	userData = nullptr;
	sock = INVALID_SOCKET;
	sequenceNumber = 0;
	memset(addr, 0, SOCKADDR_IN_SIZE);
	msgBuffer = new char[MAX_UDP_PACKET_SIZE];
	memset(msgBuffer, 0, MAX_UDP_PACKET_SIZE);
	joinCb = nullptr;
	disconnectCb = nullptr;
}
UDPServerSocket::~UDPServerSocket()
{
	delete[] msgBuffer;
}
NetError UDPServerSocket::Create(const char* ipAddr, uint16_t port)
{
	if (!StartUp()) return NetError::E_INIT;
	if (sock == INVALID_SOCKET)
	{
		sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET) return NetError::E_INIT;
	}
	SetSocketBlockingEnabled(sock, false);
	sockaddr_in* paddr = (sockaddr_in*)addr;
	paddr->sin_family = AF_INET;
	paddr->sin_addr.S_un.S_addr = inet_addr(ipAddr);
	paddr->sin_port = port;
	if (bind(sock, (sockaddr*)addr, SOCKADDR_IN_SIZE)) return NetError::E_INIT;

	return NetError::OK;
}


bool UDPServerSocket::SendDataAll(void* data, int size)
{
	bool allSucceeded = true;
	for (int i = 0; i < SERVER_MAX_PLAYERS; i++)
	{
		if (clients[i].isActive)
		{
			int sendBytes = sendto(sock, (const char*)data, size, 0, (sockaddr*)clients[i].addr, SOCKADDR_IN_SIZE);
			sequenceNumber++;
			if (sendBytes != size) allSucceeded = false;
		}
	}
	return allSucceeded;
}

bool UDPServerSocket::SendData(void* data, int size, ClientID id)
{
	if (id < SERVER_MAX_PLAYERS && clients[id].isActive)
	{
		int sendBytes = sendto(sock, (const char*)data, size, 0, (sockaddr*)clients[id].addr, SOCKADDR_IN_SIZE);
		sequenceNumber++;
		return sendBytes == size;
	}
	return true;
}
bool UDPServerSocket::SendData(void* data, int size, ClientData* client)
{
	if (client->isActive)
	{
		int sendBytes = sendto(sock, (const char*)data, size, 0, (sockaddr*)client->addr, SOCKADDR_IN_SIZE);
		sequenceNumber++;
		return sendBytes == size;
	}
	return true;
}

bool UDPServerSocket::SendImportantAll(void* data, int size)
{
	bool sendToAll = SendDataAll(data, size);
	if (sendToAll)
	{
		char* dataCopy = new char[size];
		memcpy(dataCopy, data, size);
		this->tempStorage.push_back({ dataCopy,  size, 0.0f, clientsBitset, {0} });
	}
	return sendToAll;
}

bool UDPServerSocket::SendImportantData(void* data, int size, ClientID id)
{
	bool sendSucceeded = SendData(data, size, id);
	if (sendSucceeded)
	{
		ResendPacketData resendInfo;
		resendInfo.data = new char[size];
		resendInfo.size = size;
		memcpy(resendInfo.data, data, size);
		resendInfo.finAck.set(id);
		tempStorage.push_back(resendInfo);
	}
	return sendSucceeded;
}

bool UDPServerSocket::SendImportantData(void* data, int size, ClientData* client)
{
	bool sendSucceeded = SendData(data, size, client);
	if (sendSucceeded)
	{
		ResendPacketData resendInfo;
		resendInfo.data = new char[size];
		resendInfo.size = size;
		memcpy(resendInfo.data, data, size);
		uint16_t id = GetClientID(client);
		resendInfo.finAck.set(id);
		tempStorage.push_back(resendInfo);
	}
	return sendSucceeded;
}

bool UDPServerSocket::Poll(float dt)
{
	sockaddr_in clientAddr{};
	clientAddr.sin_addr.S_un.S_addr = 0;
	clientAddr.sin_port = 0;


	int len = sizeof(clientAddr);
	int received = recvfrom(sock, msgBuffer, MAX_UDP_PACKET_SIZE, 0, (sockaddr*)&clientAddr, &len);
	if (received < MAX_UDP_PACKET_SIZE && received > 0)
	{
		ClientData* client = GetClient((sockaddr*)&clientAddr);
		BaseHeader* header = (BaseHeader*)msgBuffer;
		uint16_t id = header->packetID & ~(CLIENT_IMPORTANT_FLAG);
		if (client)
		{
			client->lastPacketTime = 0.0f;
			client->knownSequenceNumber = header->sequenceNumber;
			if (header->packetID & CLIENT_IMPORTANT_FLAG) SendAck(header->sequenceNumber, (sockaddr*)&clientAddr);
			
			if (id == CLIENT_PACKET_JOIN && received == sizeof(Client::JoinPacket))
			{
				Server::JoinResponsePacket responsePacket{ 0 };
				responsePacket.packetID = SERVER_PACKET_JOIN;
				responsePacket.sequenceNumber = sequenceNumber;
				responsePacket.id = GetClientID(client);
				if (responsePacket.id != -1)
				{
					memcpy(responsePacket.name, client->name, MAX_NAME_LENGTH);
					if (!SendImportantData(&responsePacket, sizeof(responsePacket), responsePacket.id))
					{
						LOG("FAILED TO SEND RESPONSE PACKET TO CLIENT\n");
					}
				}
			}
			else if (id == CLIENT_PACKET_ACK)
			{
				uint16_t clientID = GetClientID(client);
				if (clientID != -1)
				{
					ReceiveAck(header->sequenceNumber, clientID);
				}
			}
			else if (id == CLIENT_PACKET_DISCONNECT)
			{
				RemoveClient(client);
			}
			else if(id < packetHandlers.size() && packetHandlers.at(id))
			{
				int msgLen = packetHandlers.at(id)(this, client, msgBuffer, received);
			}

		}
		else if(received == sizeof(Client::JoinPacket) && (header->packetID & CLIENT_IMPORTANT_FLAG) && id == CLIENT_PACKET_JOIN)
		{
			SendAck(header->sequenceNumber, (sockaddr*)&clientAddr);
			
			Client::JoinPacket* pack = (Client::JoinPacket*)msgBuffer;
			Server::JoinResponsePacket responsePacket{0};
			responsePacket.packetID = SERVER_PACKET_JOIN;
			responsePacket.sequenceNumber = sequenceNumber;
			responsePacket.id = 0;
			for (int i = 0; i < MAX_NAME_LENGTH; i++)
			{
				if (pack->name[i] == '\00') break;
				responsePacket.name[i] = pack->name[i];
			}
			JOIN_ERROR nameErr = ValidatePlayerName(responsePacket.name);
			responsePacket.error = (uint16_t)nameErr;
			if (nameErr == JOIN_ERROR::JOIN_OK)
			{
				for (int i = 0; i < SERVER_MAX_PLAYERS; i++)
				{
					if (clients[i].isActive)
					{
						if (strncmp(responsePacket.name, clients[i].name, MAX_NAME_LENGTH) == 0) {
							nameErr = JOIN_ERROR::JOIN_NAME_COLLISION;
							break;
						}
					}
				}
				
				if (nameErr == JOIN_ERROR::JOIN_OK)
				{
					int id = AddClient((sockaddr*)&clientAddr);
					if (id == -1) responsePacket.error = (uint16_t)JOIN_ERROR::JOIN_FULL;
					else
					{
						Server::AddClient addClient;
						addClient.packetID = SERVER_PACKET_ADD_CLIENT | SERVER_IMPORTANT_FLAG;
						addClient.clientID = id;
						memcpy(addClient.name, clients[id].name, MAX_NAME_LENGTH);
						addClient.sequenceNumber = sequenceNumber;
						SendImportantDataAllExcept(&addClient, sizeof(Server::AddClient), id);
					}
					responsePacket.id = id;
				}
			}
			if (responsePacket.error == 0)
			{
				responsePacket.packetID |= SERVER_IMPORTANT_FLAG;
				if (!SendImportantData(&responsePacket, sizeof(responsePacket), responsePacket.id))
				{
					LOG("FAILED TO SEND RESPONSE TO CLIENT\n");
				}
			}
			else
			{
				int sendLen = sendto(sock, (char*)&responsePacket, sizeof(responsePacket), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
				if (sendLen != sizeof(responsePacket))
				{
					LOG("FAILED TO SEND RESPONSE TO CLIENT\n");
				}
			}
			
			
		}
	}

	for (int i = 0; i < tempStorage.size(); i++)
	{
		ResendPacketData& resend = tempStorage.at(i);
		std::bitset<SERVER_MAX_PLAYERS> overlapp(resend.finAck & resend.regAck);
		if (resend.finAck == overlapp)
		{
			delete[] resend.data;
			tempStorage.erase(tempStorage.begin() + i);
			i--;
			continue;
		}
		resend.accumulatedTime += dt * 100.0f;
		if (resend.accumulatedTime >= settings.resendDelay)
		{
			resend.accumulatedTime = 0.0f;
			for (int j = 0; j < resend.regAck.size(); j++)
			{
				if (!resend.regAck.test(j) && resend.finAck.test(j))
				{
					SendData(resend.data, resend.size, j);
				}
			}

		}
	}

	return true;
}

bool UDPServerSocket::SendDataAllExcept(void* data, int size, ClientID exception)
{
	bool allWorked = true;
	for(int i = 0; i < SERVER_MAX_PLAYERS; i++)
	{
		if (i == exception) continue;
		if (clients[i].isActive)
		{
			int sendBytes = sendto(sock, (const char*)data, size, 0, (sockaddr*)clients[i].addr, SOCKADDR_IN_SIZE);
			if (sendBytes != size) allWorked = false;
		}
	}
	return allWorked;
}
bool UDPServerSocket::SendImportantDataAllExcept(void* data, int size, ClientID exception)
{
	bool sendToAll = SendDataAllExcept(data, size, exception);
	if (sendToAll)
	{
		char* dataCopy = new char[size];
		memcpy(dataCopy, data, size);
		this->tempStorage.push_back({ dataCopy,  size, 0.0f, clientsBitset, {0} });
		tempStorage.at(tempStorage.size() - 1).finAck.set(exception, false);
	}
	return sendToAll;
}
bool UDPServerSocket::SendAck(uint16_t sequence, ClientID id)
{
	if (clients[id].isActive)
	{
		return SendAck(sequence, (sockaddr*)clients[id].addr);
	}
	return false;
}
void UDPServerSocket::SetUserData(void* data)
{
	userData = data;
}
void* UDPServerSocket::GetUserData()
{
	return userData;
}
void UDPServerSocket::AddPacketFunction(ServerPacketFunction fun, uint16_t packetID)
{
	if (packetHandlers.size() < packetID)
	{
		packetHandlers.resize(packetID + 1);
	}
	packetHandlers.at(packetID) = fun;
}
void UDPServerSocket::RemovePacketFunction(uint16_t packetID)
{
	AddPacketFunction(nullptr, packetID);
}
void UDPServerSocket::SetJoinCallback(JoinCallbackFunction fun)
{
	joinCb = fun;
}
void UDPServerSocket::SetDisconnectCallback(DisconnectCallbackFunction fun)
{
	disconnectCb = fun;
}
bool UDPServerSocket::SendAck(uint16_t sequence, const struct sockaddr* client)
{
	BaseHeader ack;
	ack.packetID = SERVER_PACKET_ACK;
	ack.sequenceNumber = sequence;
	int len = sendto(sock, (const char*)&ack, sizeof(ack), 0, client, SOCKADDR_IN_SIZE);
	if (len != sizeof(ack)) return false;
	return true;
}
void UDPServerSocket::ReceiveAck(uint16_t sequence, ClientID id)
{
	if (id < SERVER_MAX_PLAYERS)
	{
		for (int i = 0; i < tempStorage.size(); i++)
		{
			ResendPacketData& resend = tempStorage.at(i);
			BaseHeader* header = (BaseHeader*)resend.data;
			if (header->sequenceNumber == sequence)
			{
				resend.regAck.set(id);
			}
		}
	}
}

UDPServerSocket::ClientData* UDPServerSocket::GetClient(const sockaddr* clientAddr)
{
	for (int i = 0; i < SERVER_MAX_PLAYERS; i++)
	{
		if (!clients[i].isActive) continue;
		if (memcmp(clients[i].addr, clientAddr, SOCKADDR_IN_SIZE) == 0)
		{
			return &clients[i];
		}
	}
	return nullptr;
}
uint16_t UDPServerSocket::GetClientID(const UDPServerSocket::ClientData* client)
{
	uintptr_t idx = (uintptr_t)client - (uintptr_t)clients;
	idx /= sizeof(ClientData);
	if (idx < SERVER_MAX_PLAYERS)
	{
		return idx;
	}
	return -1;
}
int UDPServerSocket::AddClient(const struct sockaddr* clientAddr)
{
	for (int i = 0; i < SERVER_MAX_PLAYERS; i++)
	{
		if (!clients[i].isActive)
		{
			clients[i].isActive = true;
			memcpy(clients[i].addr, clientAddr, SOCKADDR_IN_SIZE);
			clients[i].knownSequenceNumber = 0;
			clients[i].lastPacketTime = 0.0f;

			if (joinCb && !joinCb(this, &clients[i])) {
				clients[i].isActive = false;
				memset(clients[i].addr, 0, SOCKADDR_IN_SIZE);
				clients[i].knownSequenceNumber = 0;
				clients[i].lastPacketTime = 0.0f;
				return -2;
			}
			
			return i;
		}
	}
	return -1;
}
void UDPServerSocket::RemoveClient(ClientData* client)
{
	if (client->isActive)
	{
		if (disconnectCb) disconnectCb(this, client);
		client->isActive = false;
		memset(client, 0, SOCKADDR_IN_SIZE);
		client->knownSequenceNumber = 0;
		client->lastPacketTime = 0.0f;
	}
}















std::string GetIPAddress(uintptr_t socket)
{
	sockaddr_in addrInfo; int nameLen;
	getsockname(socket, (sockaddr*)&addrInfo, &nameLen);
	char* ipAddr = inet_ntoa(addrInfo.sin_addr);
	std::string res = ipAddr;
	return res;
}
