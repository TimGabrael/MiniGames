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
	if (!StartUp()) return NetError::E_INIT;
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock == INVALID_SOCKET) return NetError::E_INIT;
	
	
	sockaddr_in* addr= (sockaddr_in*)serverAddr;
	addr->sin_family = AF_INET;
	addr->sin_addr.S_un.S_addr = inet_addr(host);
	addr->sin_port = port;

	return NetError::OK;
}

bool UDPSocket::SendData(void* data, int size)
{
	int numBytesWritten = sendto(sock, (const char*)data, size, 0, (sockaddr*)serverAddr, SOCKADDR_IN_SIZE);
	if (numBytesWritten != size)
	{
		LOG("[ERROR]: FAILED TO SEND DATA\n");
		return false;
	}
	sequenceNumber++;
	return true;
}

bool UDPSocket::SendImportantData(void* data, int size)
{
	char* fullMsg = new char[size];
	memcpy(fullMsg, data, size);
	if (SendData(fullMsg, size))
	{
		tempStorage.push_back({ fullMsg, size, 0.0f, 0 });
		return true;
	}
	return false;
}

bool UDPSocket::Poll(float dt)
{
	sockaddr_in server{0};
	int addrSize = SOCKADDR_IN_SIZE;
	int out = recvfrom(sock, msgBuffer, MAX_UDP_PACKET_SIZE, 0, (sockaddr*)&server, &addrSize);
	if (out > MAX_UDP_PACKET_SIZE || out < 0) return false;
	if (out >= sizeof(BaseHeader))
	{
		BaseHeader* headerInfo = (BaseHeader*)msgBuffer;
		if (headerInfo->packetID & CLIENT_IMPORTANT_FLAG)
		{
			SendAck(headerInfo->sequenceNumber);
		}
	}
	return true;
}

bool UDPSocket::SendAck(uint16_t sequence) const
{
	BaseHeader ack;
	ack.packetID = CLIENT_PACKET_ACK;
	ack.sequenceNumber = sequence;
	int len = sendto(sock, (const char*)&ack, sizeof(ack), 0, (sockaddr*)serverAddr, SOCKADDR_IN_SIZE);
	return len == sizeof(BaseHeader);
}

uint16_t UDPSocket::GetSequenceNumber() const
{
	return sequenceNumber;
}










UDPServerSocket::UDPServerSocket()
{
	sock = INVALID_SOCKET;
	sequenceNumber = 0;
	memset(addr, 0, SOCKADDR_IN_SIZE);
	msgBuffer = new char[MAX_UDP_PACKET_SIZE];
	memset(msgBuffer, 0, MAX_UDP_PACKET_SIZE);
}
UDPServerSocket::~UDPServerSocket()
{
	delete[] msgBuffer;
}
NetError UDPServerSocket::Create(const char* ipAddr, uint16_t port)
{
	if (!StartUp()) return NetError::E_INIT;
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock == INVALID_SOCKET) return NetError::E_INIT;

	sockaddr_in* paddr = (sockaddr_in*)addr;
	paddr->sin_family = AF_INET;
	paddr->sin_addr.S_un.S_addr = inet_addr(ipAddr);
	paddr->sin_port = port;
	if (bind(sock, (sockaddr*)addr, SOCKADDR_IN_SIZE)) return NetError::E_INIT;

	return NetError::OK;
}


bool UDPServerSocket::SendAll(void* data, int size)
{
	bool allSucceeded = true;
	for (int i = 0; i < SERVER_MAX_PLAYERS; i++)
	{
		if (clients[i].isActive)
		{
			int sendBytes = sendto(sock, (const char*)data, size, 0, (sockaddr*)clients[i].addr, SOCKADDR_IN_SIZE);
			if (sendBytes != size) allSucceeded = false;
		}
	}
	return allSucceeded;
}

bool UDPServerSocket::Send(void* data, int size, ClientID id)
{
	if (id < SERVER_MAX_PLAYERS && clients[id].isActive)
	{
		int sendBytes = sendto(sock, (const char*)data, size, 0, (sockaddr*)clients[id].addr, SOCKADDR_IN_SIZE);
		return sendBytes == size;
	}
	return true;
}

bool UDPServerSocket::SendImportantAll(void* data, int size)
{
	bool sendToAll = SendAll(data, size);
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
	bool sendSucceeded = Send(data, size, id);
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
			if (header->packetID & CLIENT_IMPORTANT_FLAG) SendAck(header->sequenceNumber, (sockaddr*)&clientAddr);
			
			if (id == CLIENT_PACKET_JOIN && received == sizeof(ClientJoinPacket))
			{
				Server::JoinResponsePacket responsePacket{ 0 };
				responsePacket.packetID = SERVER_PACKET_JOIN_RESPONSE;
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

				}
			}

		}
		else if(received == sizeof(ClientJoinPacket) && (header->packetID & CLIENT_IMPORTANT_FLAG) && id == CLIENT_PACKET_JOIN)
		{
			SendAck(header->sequenceNumber, (sockaddr*)&clientAddr);
			
			ClientJoinPacket* pack = (ClientJoinPacket*)msgBuffer;
			Server::JoinResponsePacket responsePacket{0};
			responsePacket.packetID = SERVER_PACKET_JOIN_RESPONSE;
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
		if (resend.finAck == resend.regAck)
		{
			tempStorage.erase(tempStorage.begin() + i);
			i--;
			continue;
		}
		resend.accumulatedTime += dt;
		if (resend.accumulatedTime >= settings.resendDelay)
		{
			resend.accumulatedTime = 0.0f;
			for (int j = 0; j < resend.regAck.size(); j++)
			{
				if (!resend.regAck.test(j) && resend.finAck.test(j))
				{
					Send(resend.data, resend.size, j);
				}
			}

		}
	}

	return true;
}

bool UDPServerSocket::SendAck(uint16_t sequence, ClientID id)
{
	if (clients[id].isActive)
	{
		return SendAck(sequence, (sockaddr*)clients[id].addr);
	}
	return false;
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
			return i;
		}
	}
	return -1;
}
















std::string GetIPAddress(uintptr_t socket)
{
	sockaddr_in addrInfo; int nameLen;
	getsockname(socket, (sockaddr*)&addrInfo, &nameLen);
	char* ipAddr = inet_ntoa(addrInfo.sin_addr);
	std::string res = ipAddr;
	return res;
}
