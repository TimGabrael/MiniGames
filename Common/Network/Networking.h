#pragma once
#include <stdint.h>
#include <thread>
#include <vector>
#include <mutex>
#include "Cryptography.h"
#include "NetworkBase.h"
#include <functional>
#include <bitset>


#define MAX_UDP_PACKET_SIZE 0xFFFF
#define SOCKADDR_IN_SIZE 16
#define SERVER_MAX_PLAYERS 0xFF

// if this is changed, KEY_LEN needs to be changed as well !
#define KEY_BITS_512
#ifdef KEY_BITS_128
typedef uint128_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU128;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU128;
#elif defined(KEY_BITS_256)
typedef uint256_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU256; 
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU256;
#elif defined(KEY_BITS_512)
typedef uint512_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU512;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU512;
#elif defined(KEY_BITS_1024)
typedef uint1024_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU1024;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU1024;
#elif defined(KEY_BITS_2048)
typedef uint2048_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU2048;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU2048;
#elif defined(KEY_BITS_4096)
typedef uint4096_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU4096;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU4096;
#endif



class UDPSocket
{
public:
	UDPSocket();
	~UDPSocket();

	NetError Create(const char* host, uint16_t port);
	NetError Connect(const char* host, uint16_t port, const std::string& name);

	bool SendData(void* data, int size);
	
	// repeats the action until an ack was sent back, is non blocking
	bool SendImportantData(void* data, int size);

	bool Poll(float dt);


	uint16_t GetSequenceNumber() const;

	void SetUserData(void* data);
	void* GetUserData();

	void AddPacketFunction(ClientPacketFunction fun, uint16_t packetID);
	void RemovePacketFunction(uint16_t packetID);

private:
	bool SendAck(uint16_t sequence) const;

	void ReceiveAck(uint16_t sequence);

	struct ResendPacketData
	{
		char* data = nullptr;
		int size = 0;
		float accumulatedTime = 0.0f;
		uint16_t knownSequenceNumber = 0;
	};
	void* userData;
	uintptr_t sock;
	uint16_t sequenceNumber;
	uint8_t serverAddr[SOCKADDR_IN_SIZE];
	char* msgBuffer;
	std::vector<ResendPacketData> tempStorage;
	std::vector<ClientPacketFunction> packetHandlers;

};

typedef uint16_t ClientID;
class UDPServerSocket
{
public:
	struct ClientData
	{
		char name[MAX_NAME_LENGTH + 1];
		float lastPacketTime = 0.0f;
		uint16_t knownSequenceNumber = 0;
		uint8_t addr[SOCKADDR_IN_SIZE] = { 0 };
		bool isActive = false;
	};
	UDPServerSocket();
	~UDPServerSocket();


	NetError Create(const char* ipAddr, uint16_t port);
	bool Poll(float dt);


	bool SendDataAll(void* data, int size);
	bool SendData(void* data, int size, ClientID id);
	bool SendData(void* data, int size, ClientData* client);

	bool SendImportantAll(void* data, int size);
	bool SendImportantData(void* data, int size, ClientID id);
	bool SendImportantData(void* data, int size, ClientData* client);
	
	bool SendAck(uint16_t sequence, ClientID id);

	void SetUserData(void* data);
	void* GetUserData();

	void AddPacketFunction(ServerPacketFunction fun, uint16_t packetID);
	void RemovePacketFunction(uint16_t packetID);

	void SetJoinCallback(JoinCallbackFunction fun);
	void SetDisconnectCallback(DisconnectCallbackFunction fun);


	ClientData clients[SERVER_MAX_PLAYERS];


	bool SendDataAllExcept(void* data, int size, ClientID exception);
	bool SendImportantDataAllExcept(void* data, int size, ClientID exception);

	bool SendAck(uint16_t sequence, const struct sockaddr* client);
	void ReceiveAck(uint16_t sequence, ClientID id);
	


	struct ResendPacketData
	{
		char* data = nullptr;
		int size = 0;
		float accumulatedTime = 0.0f;
		std::bitset<SERVER_MAX_PLAYERS> finAck = {0};
		std::bitset<SERVER_MAX_PLAYERS> regAck = {0};
	};

	ClientData* GetClient(const struct sockaddr* clientAddr);
	uint16_t GetClientID(const ClientData* client);
	int AddClient(const struct sockaddr* clientAddr);
	void RemoveClient(ClientData* client);
	
	void* userData;
	uintptr_t sock;
	JoinCallbackFunction joinCb;
	DisconnectCallbackFunction disconnectCb;
	uint16_t sequenceNumber;
	uint8_t addr[SOCKADDR_IN_SIZE];
	std::bitset<SERVER_MAX_PLAYERS> clientsBitset = {0};
	char* msgBuffer;
	std::vector<ResendPacketData> tempStorage;
	std::vector<ServerPacketFunction> packetHandlers;
};

