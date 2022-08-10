#pragma once
#include <stdint.h>
#include <thread>
#include <vector>
#include <mutex>
#include "Cryptography.h"
#include "NetworkBase.h"

struct ValidationPacket
{
	uint8_t PacketData[64];
};

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




class TCPSocket;
class TCPServerSocket;
typedef void(*ConnectionFunc)(void* obj, struct Connection* conn);
typedef void(*ClientPacketCallback)(void* userData, Packet* pack);
typedef void(*ServerPacketCallback)(void* userData, Connection* conn, Packet* pack);
typedef void(*ConnectCallback)(void* userData, TCPServerSocket* sock, Connection* conn);
typedef void(*DisconnectCallback)(void* userData, TCPServerSocket* sock, Connection* removed);
typedef void(*ClientDisconnectCallback)(void* userData, TCPSocket* sock, Connection* removed);

struct Connection
{
	Connection();
	Connection(uintptr_t sock);
	Connection(uintptr_t sock, ConnectionFunc socketCallback);
	Connection(uintptr_t sock, ConnectionFunc socketCallback, void* obj);
	Connection(uintptr_t sock, ConnectionFunc socketCallback, void* obj, const ukey_t& key);
	~Connection();

	void Init(uintptr_t sock, ConnectionFunc socketCallback, void* obj, const ukey_t& key);

	void SendData(PacketID id, uint32_t group, uint16_t additionalFlags, uint16_t clientID, size_t size, const void* data);
	void SendData(PacketID id, uint32_t group, uint16_t additionalFlags, uint16_t clientID, const std::string& str);

	void SetCryptoKey(const ukey_t& k);

	CryptoContext ctx;
	uintptr_t socket;
	std::thread listener;
	Packet storedPacket;
};

class TCPSocket
{
public:
	TCPSocket();
	~TCPSocket() { running = false; }
	NetError Connect(const char* host, const char* port);
	void Disconnect();

	void SetPacketCallback(ClientPacketCallback cb, void* userData = nullptr);
	void SetDisconnectCallback(ClientDisconnectCallback cb, void* userData = nullptr);

	void SendData(PacketID id, uint32_t group, uint16_t additionalFlags, uint16_t clientID, size_t size, const void* data);
	void SendData(PacketID id, uint32_t group, uint16_t additionalFlags, uint16_t clientID, const std::string& str);

private:
	void SendDataUnencrypted(PacketID id, uint32_t size, const uint8_t* data);
	static void SocketListenFunc(void* obj, struct Connection* conn);
	uintptr_t sock;
	ClientPacketCallback packCb = nullptr;
	void* userDataPacketCB = nullptr;
	ClientDisconnectCallback disconnectCB = nullptr;
	void* userDataDisconnectCB = nullptr;
	Connection serverConn;
	ukey_t privateKey;
	ukey_t publicKey;
	bool running = true;
};

class TCPServerSocket
{
public:
	TCPServerSocket();
	NetError Create(const char* host, const char* port);
	NetError AcceptConnection();

	void SetConnectCallback(ConnectCallback cb, void* userData = nullptr);
	void SetPacketCallback(ServerPacketCallback cb, void* userData = nullptr);
	void SetDisconnectCallback(DisconnectCallback cb, void* userData = nullptr);

	void RemoveAll();

	void RemoveClient(Connection* conn);

	static void TCPServerListenToClient(void* server, Connection* conn);
private:


	void AddClient(uintptr_t connSocket, const ukey_t& key);
	void RemoveClient(uintptr_t connSocket);
	void RemoveClientAtIdx(size_t idx);


	ServerPacketCallback packetCallback;
	void* userDataPacketCB;
	ConnectCallback connectCallback;
	void* userDataConnectCB;
	DisconnectCallback disconnectCallback;
	void* userDataDisconnectCB;
	uintptr_t sock;
	std::mutex clientMut;
	std::vector<Connection*> clients;
	ukey_t privateKey;
	ukey_t publicKey;
	bool running;
};

std::string GetIPAddress(uintptr_t socket);
